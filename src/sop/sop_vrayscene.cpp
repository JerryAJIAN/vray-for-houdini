//
// Copyright (c) 2015-2016, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#ifdef CGR_HAS_VRAYSCENE

#include "sop_vrayscene.h"
#include "gu_vraysceneref.h"

#include <GEO/GEO_Point.h>
#include <GU/GU_PrimPoly.h>
#include <OP/OP_Options.h>

using namespace VRayForHoudini;


VUtils::Vrscene::Preview::VrsceneDescManager  SOP::VRayScene::m_vrsceneMan;


void SOP::VRayScene::setPluginType()
{
	pluginType = VRayPluginType::GEOMETRY;
	pluginID   = "VRayScene";
}


VUtils::Vrscene::Preview::VrsceneSettings SOP::VRayScene::getVrsceneSettings() const
{
	VUtils::Vrscene::Preview::VrsceneSettings settings;
	settings.usePreview = true;
	settings.previewFacesCount = 100000;
	settings.cacheSettings.cacheType = (VUtils::Vrscene::Preview::VrsceneCacheSettings::VrsceneCacheType)2;
	return settings;
}


OP_ERROR SOP::VRayScene::cookMySop(OP_Context &context)
{
	Log::getLog().debug("SOP::VRayScene::cookMySop()");

	const fpreal t = context.getTime();
	
	gdp->stashAll();

	UT_String path;
	evalString(path, "filepath", 0, t);
	if (path.equal("")) {
		gdp->destroyStashed();
		return error();
	}

	if (error() < UT_ERROR_ABORT) {
		UT_Interrupt *boss = UTgetInterrupt();

		const bool is_animated = evalInt("anim_type", 0, t) != 3;
		flags().setTimeDep(is_animated);

		if (boss) {
			if (boss->opStart("Building V-Ray Scene Preview Mesh")) {
				GU_PrimPacked *pack = GU_PrimPacked::build(*gdp, "VRaySceneRef");
				if (NOT(pack)) {
					addWarning(SOP_MESSAGE, "Can't create packed primitive VRaySceneRef");
				}
				else {
					// Set the location of the packed primitive's point.
					UT_Vector3 pivot(0, 0, 0);
					pack->setPivot(pivot);
					gdp->setPos3(pack->getPointOffset(0), pivot);

					OP_Options options;
					for (int i = 0; i < getParmList()->getEntries(); ++i) {
						const PRM_Parm &prm = getParm(i);
						options.setOptionFromTemplate(this, prm, *prm.getTemplatePtr(), t);
					}

					const PRM_Parm &prm = getParm(VRaySceneRef::mw_shadow_tint_color_param_name);
					options.setOptionFromTemplate(this, prm, *prm.getTemplatePtr(), t);

					pack->implementation()->update(options);
					pack->setPathAttribute(getFullPath());
				}

				gdp->destroyStashed();

				using namespace VUtils;
				const Vrscene::Preview::VrsceneSettings &vrsceneSettings = getVrsceneSettings();

				Vrscene::Preview::VrsceneDesc *vrsceneDesc = m_vrsceneMan.getVrsceneDesc(path.buffer(), &vrsceneSettings);
				if (vrsceneDesc) {
					const bool flipAxis = evalInt("flip_axis", 0, 0.0);

					unsigned meshVertexOffset = 0;

					for (Vrscene::Preview::VrsceneObjects::iterator obIt = vrsceneDesc->m_objects.begin(); obIt != vrsceneDesc->m_objects.end(); ++obIt) {
						Vrscene::Preview::VrsceneObjectBase *ob = obIt.data();
						if (ob && ob->getType() == Vrscene::Preview::ObjectTypeNode) {
							const VUtils::TraceTransform &tm = ob->getTransform(t);

							Vrscene::Preview::VrsceneObjectNode     *node = static_cast<Vrscene::Preview::VrsceneObjectNode*>(ob);
							Vrscene::Preview::VrsceneObjectDataBase *nodeData = vrsceneDesc->getObjectData(node->getDataName().ptr());
							if (nodeData) {
								if (nodeData->getDataType() == Vrscene::Preview::ObjectDataTypeMesh) {
									Vrscene::Preview::VrsceneObjectDataMesh *mesh = static_cast<Vrscene::Preview::VrsceneObjectDataMesh*>(nodeData);

									const VUtils::VectorRefList &vertices = mesh->getVertices(t);
									const VUtils::IntRefList    &faces = mesh->getFaces(t);
									for (int v = 0; v < vertices.count(); ++v) {
										VUtils::Vector vert = tm * vertices[v];
										if (flipAxis) {
											vert = VUtils::Vrscene::Preview::flipMatrix * vert;
										}

										GA_Offset pointOffs = gdp->appendPoint();
										gdp->setPos3(pointOffs, UT_Vector3(vert.x, vert.y, vert.z));
									}

									for (int f = 0; f < faces.count(); f += 3) {
										GU_PrimPoly *poly = GU_PrimPoly::build(gdp, 3, GU_POLY_CLOSED, 0);
										for (int c = 0; c < 3; ++c) {
											poly->setVertexPoint(c, meshVertexOffset + faces[f + c]);
										}
										poly->reverse();
									}

									meshVertexOffset += vertices.count();
								}
							}
						}
					}
				}
			}
			boss->opEnd();
		}
	}

	return error();
}


OP::VRayNode::PluginResult SOP::VRayScene::asPluginDesc(Attrs::PluginDesc &pluginDesc, VRayExporter &exporter, ExportContext *parentContext)
{
	UT_String path;
	evalString(path, "filepath", 0, 0.0f);
	if (NOT(path.isstring())) {
		Log::getLog().error("VRayScene \"%s\": \"Filepath\" is not set!",
							getName().buffer());
		return OP::VRayNode::PluginResultError;
	}

	const bool flipAxis = evalInt("flip_axis", 0, 0.0);

	pluginDesc.pluginID   = pluginID.c_str();
	pluginDesc.pluginName = VRayExporter::getPluginName(parentContext->getTarget()->castToOBJNode());

	pluginDesc.addAttribute(Attrs::PluginAttr("filepath", path.buffer()));

	VUtils::TraceTransform tm = toVutilsTm(VRayExporter::getObjTransform(parentContext->getTarget()->castToOBJNode(), exporter.getContext()));
	if (flipAxis) {
		tm.m = tm.m * VUtils::Vrscene::Preview::flipMatrix;
	}

	pluginDesc.addAttribute(Attrs::PluginAttr("transform", toAppSdkTm(tm)));

	exporter.setAttrsFromOpNodePrms(pluginDesc, this);

	return OP::VRayNode::PluginResultSuccess;
}

#endif // CGR_HAS_VRAYSCENE

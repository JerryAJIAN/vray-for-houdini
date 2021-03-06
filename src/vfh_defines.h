//
// Copyright (c) 2015-2018, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#ifndef VRAY_FOR_HOUDINI_UTIL_DEFINES_H
#define VRAY_FOR_HOUDINI_UTIL_DEFINES_H

#include "vfh_vray.h"

// QT defines macro "foreach" to be the same as QT_FOREACH which is highly likely to collide
// openvdb/util/NodeMask has "foreach" method
// define this macro to stop QT from defining it
#ifndef QT_NO_KEYWORDS
#  define QT_NO_KEYWORDS
#endif
#ifdef Q_FOREACH
#  undef Q_FOREACH
#endif

#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define NOT(x) !(x)

#define MemberEq(member) (member == other.member)
#define MemberFloatEq(member) (IsFloatEq(member, other.member))
#define MemberNotEq(member) (!(member == other.member))

#define FOR_IT(type, itName, var) int itName##Idx = 0; for (type::iterator itName = (var).begin(); itName != (var).end(); ++itName, ++itName##Idx)
#define FOR_CONST_IT(type, itName, var) int itName##Idx = 0; for (type::const_iterator itName = (var).begin(); itName != (var).end(); ++itName, ++itName##Idx)

template <typename T>
FORCEINLINE void FreePtr(T* &p) {
	delete p;
	p = nullptr;
}

template <typename T>
FORCEINLINE void FreePtrArr(T* &p) {
	delete [] p;
	p = nullptr;
}

template <typename A>
FORCEINLINE bool IsFloatEq(const A, const A) { vassert(false); return false; }
template <>
FORCEINLINE bool IsFloatEq(const float a, const float b) { return VUtils::isZero(a - b, 1e-6f); }
template <>
FORCEINLINE bool IsFloatEq(const double a, const double b) { return VUtils::isZero(a - b, 1e-6); }

#endif // VRAY_FOR_HOUDINI_UTIL_DEFINES_H

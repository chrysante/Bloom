#ifndef BLOOM_CORE_SERIALIZE_H
#define BLOOM_CORE_SERIALIZE_H

#include <utl/common.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Core/Yaml.h"

/// MARK: Serialize Fields
#define _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                 \
    static Node encode(Type const& t) {                                        \
        Node _node;

#define _BLOOM_TEXT_SERIALIZE_FIELD(FieldName) _node[#FieldName] = t.FieldName

#define _BLOOM_TEXT_SERIALIZE_FUNC_END()                                       \
    return _node;                                                              \
    }

/// MARK: Deserialize Fields
#define _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                               \
    static bool decode(Node const& _node, Type& t) {                           \
        try {

#define _BLOOM_TEXT_DESERIALIZE_FIELD(FieldName)                               \
    t.FieldName = _node[#FieldName].as<decltype(t.FieldName)>()

#define _BLOOM_TEXT_DESERIALIZE_FUNC_END()                                     \
    return true;                                                               \
    }                                                                          \
    catch (InvalidNode const&) {                                               \
        return false;                                                          \
    }                                                                          \
    }

/// MARK: Arities
#define _BLOOM_REGISTER_TEXT_SERIALIZER(Type, ...)                             \
    UTL_VFUNC(_BLOOM_REGISTER_TEXT_SERIALIZER_, Type, __VA_ARGS__)

#define _BLOOM_REGISTER_TEXT_SERIALIZER_2(Type, _0)                            \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_3(Type, _0, _1)                        \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_4(Type, _0, _1, _2)                    \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_5(Type, _0, _1, _2, _3)                \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_6(Type, _0, _1, _2, _3, _4)            \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_7(Type, _0, _1, _2, _3, _4, _5)        \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_8(Type, _0, _1, _2, _3, _4, _5, _6)    \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_9(Type, _0, _1, _2, _3, _4, _5, _6,    \
                                          _7)                                  \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_10(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8)                             \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_11(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9)                         \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_12(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10)                    \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_13(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11)               \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_14(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12)          \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_15(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13)     \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_16(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14)                                \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_17(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15)                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_18(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15, _16)                      \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_16);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_19(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15, _16, _17)                 \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_16);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_17);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_20(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15, _16, _17, _18)            \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_16);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_17);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_18);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_21(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15, _16, _17, _18, _19)       \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_16);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_17);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_18);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_19);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_22(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15, _16, _17, _18, _19, _20)  \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_16);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_17);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_18);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_19);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_20);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_23(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15, _16, _17, _18, _19, _20,  \
                                           _21)                                \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_16);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_17);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_18);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_19);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_20);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_21);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_24(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15, _16, _17, _18, _19, _20,  \
                                           _21, _22)                           \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_16);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_17);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_18);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_19);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_20);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_21);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_22);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_25(Type, _0, _1, _2, _3, _4, _5, _6,   \
                                           _7, _8, _9, _10, _11, _12, _13,     \
                                           _14, _15, _16, _17, _18, _19, _20,  \
                                           _21, _22, _23)                      \
    _BLOOM_TEXT_SERIALIZE_FUNC_BEGIN(Type)                                     \
    _BLOOM_TEXT_SERIALIZE_FIELD(_0);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_1);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_2);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_3);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_4);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_5);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_6);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_7);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_8);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_9);                                           \
    _BLOOM_TEXT_SERIALIZE_FIELD(_10);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_11);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_12);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_13);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_14);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_15);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_16);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_17);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_18);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_19);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_20);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_21);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_22);                                          \
    _BLOOM_TEXT_SERIALIZE_FIELD(_23);                                          \
    _BLOOM_TEXT_SERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER(Type, ...)                           \
    UTL_VFUNC(_BLOOM_REGISTER_TEXT_DESERIALIZER_, Type, __VA_ARGS__)

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_2(Type, _0)                          \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_3(Type, _0, _1)                      \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_4(Type, _0, _1, _2)                  \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_5(Type, _0, _1, _2, _3)              \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_6(Type, _0, _1, _2, _3, _4)          \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_7(Type, _0, _1, _2, _3, _4, _5)      \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_8(Type, _0, _1, _2, _3, _4, _5, _6)  \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_9(Type, _0, _1, _2, _3, _4, _5, _6,  \
                                            _7)                                \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_10(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8)                           \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_11(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9)                       \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_12(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10)                  \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_13(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11)             \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_14(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12)        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_15(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13)   \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_16(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14)                              \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_17(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15)                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_18(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15, _16)                    \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_16);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_19(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15, _16, _17)               \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_16);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_17);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_20(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15, _16, _17, _18)          \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_16);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_17);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_18);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_21(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15, _16, _17, _18, _19)     \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_16);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_17);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_18);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_19);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_22(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15, _16, _17, _18, _19,     \
                                             _20)                              \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_16);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_17);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_18);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_19);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_20);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_23(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15, _16, _17, _18, _19,     \
                                             _20, _21)                         \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_16);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_17);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_18);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_19);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_20);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_21);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_24(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15, _16, _17, _18, _19,     \
                                             _20, _21, _22)                    \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_16);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_17);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_18);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_19);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_20);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_21);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_22);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_DESERIALIZER_25(Type, _0, _1, _2, _3, _4, _5, _6, \
                                             _7, _8, _9, _10, _11, _12, _13,   \
                                             _14, _15, _16, _17, _18, _19,     \
                                             _20, _21, _22, _23)               \
    _BLOOM_TEXT_DESERIALIZE_FUNC_BEGIN(Type)                                   \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_0);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_1);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_2);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_3);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_4);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_5);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_6);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_7);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_8);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_9);                                         \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_10);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_11);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_12);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_13);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_14);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_15);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_16);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_17);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_18);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_19);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_20);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_21);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_22);                                        \
    _BLOOM_TEXT_DESERIALIZE_FIELD(_23);                                        \
    _BLOOM_TEXT_DESERIALIZE_FUNC_END()

#define _BLOOM_REGISTER_TEXT_SERIALIZER_BEGIN(Type)                            \
    template <>                                                                \
    struct YAML::convert<Type> {
#define _BLOOM_REGISTER_TEXT_SERIALIZER_END() }

#define BLOOM_MAKE_TEXT_SERIALIZER(Type, ...)                                  \
    _BLOOM_REGISTER_TEXT_SERIALIZER_BEGIN(Type)                                \
    _BLOOM_REGISTER_TEXT_SERIALIZER(Type, __VA_ARGS__)                         \
    _BLOOM_REGISTER_TEXT_DESERIALIZER(Type, __VA_ARGS__)                       \
    _BLOOM_REGISTER_TEXT_SERIALIZER_END()

#endif // BLOOM_CORE_SERIALIZE_H

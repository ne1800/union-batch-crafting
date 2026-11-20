// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/alchemy/alchemy_extra_dependencies.hpp"
// Own metadata; no game bodies. Derivation and REA-IDs in the private handoff.
namespace ubc::cool_predebit::profile {
using Range = alchemy_flask_runtime::CodeRange;
using Kind = alchemy_flask_runtime::RangeKind;
inline constexpr Range object_created{
    0x5aaeb0, 285,
    lhiver::digest_literal("15aa6edef94ce2bacf97a3071ae52147a9d7dbead0f29bc1a0170ff70c85d4ee"),
    Kind::complete_function};
// G2A, Union.patch «Allow undeclarated instances»: the jump at 0x5aaf1d always
// takes the existing branch to the epilogue at 0x5aafbb. Hash all 285 bytes.
inline constexpr auto union_object_created_sha256 =
    lhiver::digest_literal("aa7b7b4bc329765f320a9b9914075ef8e919c649658f5bae37e7f275f3617cc6");
inline constexpr std::array<unsigned, 8> split_engine_indices{{0, 1, 4, 5, 7, 8, 12, 13}};
inline constexpr std::array<unsigned, 10> constructor_field_indices{
    {0, 1, 2, 3, 4, 8, 9, 11, 12, 13}};
inline constexpr std::array<Range, 48> engine{{
    {0x46ab00, 272,
     lhiver::digest_literal("f71752a6094f5e96d72d3eae20de85bb884631a699e326d849a7cd12955518e0"),
     Kind::complete_function}, // zSTRING::Upper
    {0x65e5a0, 648,
     lhiver::digest_literal("fd1e0782aff4900874ac617ce540339086e14a513e1e5a15f3b9e4a08b32b6ef"),
     Kind::complete_function}, // ApplyDamages
    {0x61ce50, 240,
     lhiver::digest_literal("70dffe6671e4430d3146e0f9eb411419343f1b99cd7d1e65d98144944d726ac0"),
     Kind::complete_function}, // zCVob::SetCollDetStat
    {0x61d190, 48,
     lhiver::digest_literal("91802147723d4dff92d03c9674f0633506071ea07e51e7c82fce0e6e2e0d48fc"),
     Kind::complete_function}, // zCVob::SetPhysicsEnabled
    {0x514d80, 123,
     lhiver::digest_literal("69409eb17d85dbf8509fae78a46c3d98ed44540899e6ae8a349178beeea3b2f4"),
     Kind::complete_function}, // zMAT4::operator=
    {0x5ffdd0, 51,
     lhiver::digest_literal("b1d96e02f1568245fd489105ab1b1e0cf31b0012a14a63813c24df3df5a434c6"),
     Kind::complete_function}, // zCVob::SetVobName
    {0x5fe1e0, 598,
     lhiver::digest_literal("476c578d42cd0a3e56040b2a185c15e454949c4815dfd100b4876ae2a8b78f09"),
     Kind::complete_function}, // zCVob::zCVob
    {0x517b50, 258,
     lhiver::digest_literal("6ad8ed81301534883ac76e7129b135251806bfd309a61df6237e251689ccfb83"),
     Kind::complete_function}, // Alg_Identity3D
    {0x61cf40, 244,
     lhiver::digest_literal("f0f0986dea0772a442a371b1d3e674bb814b931bbc2bbda277d4db9463d9253b"),
     Kind::complete_function}, // zCVob::SetCollDetDyn
    {0x5a9ce0, 667,
     lhiver::digest_literal("fb3e06ab74b2dc2012500f7f074a3e4100f5d43e0f2c330b875475bd3ab6f035"),
     Kind::complete_function}, // zCObject::SetObjectName
    object_created,
    {0x403080, 280,
     lhiver::digest_literal("efd51d295c738e60a44bf30b75cb22943e71adaf134e3d3bafd3bf2dee40021d"),
     Kind::complete_function}, // _Grow
    {0x403280, 71,
     lhiver::digest_literal("bacc3503ef23fa1abbaea7e9c009566eec61f8f633109f66ab7f1c9e4b9dc679"),
     Kind::complete_function}, // _Tidy
    {0x4032e0, 89,
     lhiver::digest_literal("a67fa678cf514c25a5afc571d7bb5529dc005421de9fb3a7e25113ca50a3ee5a"),
     Kind::complete_function}, // _Copy_entry
    {0x403339, 33,
     lhiver::digest_literal("22eb336b6acacadeccdca1e428825f379cd18560734c4f100ddecec55ddae645"),
     Kind::complete_function}, // _Copy_Catch
    {0x403360, 135,
     lhiver::digest_literal("1582128237e7517762e132931b077c1cb8646838b921a57a02326d961246e010"),
     Kind::complete_function}, // _Copy_continuation
    {0x565f60, 5,
     lhiver::digest_literal("75c8dba7dc263512856c1ed720c5557728bbbc3eab73e9486562839243c9abcc"),
     Kind::complete_function}, // delete_entry
    {0x7b446f, 5,
     lhiver::digest_literal("92e331aabd288d8b13ca7e9302c9368853b4405c90fa727e183a9602af18116a"),
     Kind::complete_function}, // delete_thunk
    {0x7b44cc, 6,
     lhiver::digest_literal("8fec4df9e97f631ec5bf3d149f706e5dba2708a76807b45ea9cb07f518f4e97d"),
     Kind::complete_function}, // delete_iat_thunk
    {0x7d5940, 132,
     lhiver::digest_literal("0d2e7ba2802652e4517aec0af8ad2ea5809f9077c7a301a4adb42c2e6d3c789d"),
     Kind::complete_function}, // _strcmp
    {0x7cfed3, 94,
     lhiver::digest_literal("c19dd221484d62fe60012d5924e54a4d1075bc679ec70a5d3e447026a1d63280"),
     Kind::complete_function}, // __ArrayUnwind
    {0x7a5070, 24,
     lhiver::digest_literal("b51a05d387205a1b0e9e705be47e0e4faabd95a989e7bd9318de6e92798c90b1"),
     Kind::complete_function}, // zCPar_DataStack::Pop
    {0x403060, 17,
     lhiver::digest_literal("8c18bd98a0c2ed4a3c10654c32c89a978e5e12f04546f47aa9a09cd957a31b4a"),
     Kind::
         complete_function}, // std::basic_string<char,std::char_traits<char>,std::allocator<char>_>::_Eos
    {0x402bb0, 282,
     lhiver::digest_literal("d761535afeed3422cd9889443fa1a99033a8881f8737845bdd456f256da4a898"),
     Kind::
         complete_function}, // std::basic_string<char,std::char_traits<char>,std::allocator<char>_>::erase
    {0x46c920, 251,
     lhiver::digest_literal("720b5e8ac81d700785c307a41dfe6d9349e234fa310e72ea044973c44ff18b84"),
     Kind::complete_function}, // zSTRING::Search
    {0x7d0ba2, 26,
     lhiver::digest_literal("b206e75e4cc36ca3fc0f2591db372bf78ed149fb28c4d7f7ebb054cf1f30af49"),
     Kind::complete_function}, // FUN_007d0ba2
    {0x7a1e10, 12,
     lhiver::digest_literal("4b33645950d5fa7ab0d2618c96e83e8a7ad86c161519fa7ecb70fe8e5c2e646c"),
     Kind::complete_function}, // zCPar_Symbol::GetStackPos
    {0x7a4c20, 10,
     lhiver::digest_literal("435e522bba9f73703a01d6c1dfedc9927d5bbd0483253007f2157b6e504c5989"),
     Kind::complete_function}, // zCPar_Stack::PopByte
    {0x409ff0, 284,
     lhiver::digest_literal("98668be874bfbd1d8395ac709b257ae7136d0742957908b90ed45933e7a03f45"),
     Kind::
         complete_function}, // std::basic_string<char,std::char_traits<char>,std::allocator<char>_>::basic_string<char,std::char_traits<char>,std::allocator<char>_>
    {0x7d0ed4, 24,
     lhiver::digest_literal("8b58d2dfc6639171f37d4a495bcb303f459a955778b344cfeac9c44b3744f03a"),
     Kind::complete_function}, // FUN_007d0ed4
    {0x4031a0, 213,
     lhiver::digest_literal("48bd6c3804ab8359b64cdadea1ebf8c2009d34ae66f9f6ba830802aa5ac36b35"),
     Kind::
         complete_function}, // std::basic_string<char,std::char_traits<char>,std::allocator<char>_>::_Split
    {0x4032d0, 6,
     lhiver::digest_literal("4b17618fbe68851520d85f217e1c92596a7190185923fed7dcbf852772724224"),
     Kind::
         complete_function}, // std::basic_string<char,std::char_traits<char>,std::allocator<char>_>::max_size
    {0x402dc0, 510,
     lhiver::digest_literal("417d43fb0980a28b6361301251e2142c4a9c9885bb35810c0948a8727b5a341e"),
     Kind::
         complete_function}, // std::basic_string<char,std::char_traits<char>,std::allocator<char>_>::assign
    {0x61e610, 64,
     lhiver::digest_literal("54f030281ad2ce2205b406c1f716f389c477ee2d2c97d0ef0706f987e1da3c2a"),
     Kind::complete_function}, // zCVob::SetCollisionClass
    {0x7937f0, 185,
     lhiver::digest_literal("4d931fba9f02bb327b69a8c657922988c96465ea3b03235b30f83fcf82d3821b"),
     Kind::complete_function}, // zCParser::GetSymbolInfo
    {0x7d0e6a, 106,
     lhiver::digest_literal("9f1cac68bdea576ee6283b14cacf997e25bdb5029fcd53b7126930e1d51475ca"),
     Kind::complete_function}, // `eh_vector_constructor_iterator'
    {0x401160, 67,
     lhiver::digest_literal("f9a8f22b98a5b3a5806030de83a907cd59cd9fc188f88a7c5ecbcc8e44132ace"),
     Kind::complete_function}, // zSTRING::~zSTRING
    {0x7d2bd0, 165,
     lhiver::digest_literal("0e6910ad40067aac7452d3a10910565b2cbdfb4f6232aabd05738c34e4c87b70"),
     Kind::complete_function}, // _memchr
    {0x791910, 33,
     lhiver::digest_literal("ba5d3f456384d70e5247d1949c2604b43cc2652e177a301af9edc4d7b275231a"),
     Kind::complete_function}, // zCParser::PopVarAddress
    {0x7a4f80, 225,
     lhiver::digest_literal("bd28500c0df0d6431eacbb2427914ff5762ed4be6f7b6c5d06b1e459bb7cc1e0"),
     Kind::complete_function}, // zCPar_DataStack::Push
    {0x7d0bbc, 90,
     lhiver::digest_literal("016affa34666fa53132e7d2efbfe1baec146346d122115105066e03a745d0188"),
     Kind::complete_function}, // FUN_007d0bbc
    {0x7928d0, 250,
     lhiver::digest_literal("9113b60eafc0ead2d94413e015427544dd50098db579f4e1351cd73ca69302f1"),
     Kind::complete_function}, // zSTRING::zSTRING
    {0x7d0a3a, 249,
     lhiver::digest_literal("dd79fbc9422d56021f5994a091e9ebef2901a9c014b8aa2e0186fa42e336e540"),
     Kind::complete_function}, // ___RTDynamicCast
    {0x7a1a40, 724,
     lhiver::digest_literal("346405004722d67e3462e048a978bfed3b9b2cf09f54a470ecb6631d01cbb013"),
     Kind::complete_function}, // zCPar_Symbol::GetDataAdr
    {0x7d0e47, 35,
     lhiver::digest_literal("582c3d3438d65707b6aa9e6e4fe328a57347234cb5e26dc828dc57dac052bd4a"),
     Kind::complete_function}, // FUN_007d0e47
    {0x40a110, 192,
     lhiver::digest_literal("130e5ce5e6407eabc34efbfa0b1fe001b4ac29abb562262ef71b542b1b660e01"),
     Kind::
         complete_function}, // std::basic_string<char,std::char_traits<char>,std::allocator<char>_>::basic_string<char,std::char_traits<char>,std::allocator<char>_>
    {0x7a4c40, 12,
     lhiver::digest_literal("71868d429e6bd4c14303e4308c7f05fdff80654fd9fe4bbd3043465b29f69ee8"),
     Kind::complete_function}, // zCPar_Stack::PopInt
    {0x402af0, 29,
     lhiver::digest_literal("3d16429a918656fd2e4d5eef86fe4d8c379780e276d906e0f07a2019f9462494"),
     Kind::complete_function}, // zSTRING::zSTRING
}};
inline constexpr std::array<Range, 13> data{{
    {0x82f100, 4,
     lhiver::digest_literal("3d9a42796cbb1c0528326d8b91d27b3998c30864829756a428311c67087b1e6f"),
     Kind::reviewed_site}, // Factory RTTI slot
    {0x845658, 20,
     lhiver::digest_literal("332853a19be46336d6dc99df147c83501e0eb2b05e9db812963eb2e7cf744f7b"),
     Kind::reviewed_site}, // Factory RTTI CompleteObjectLocator
    {0x89022c, 30,
     lhiver::digest_literal("fb508b6b0814917e08870b0e3435dbadb5646fd02c3827361665259a59a68dc3"),
     Kind::reviewed_site}, // Factory RTTI TypeDescriptor
    {0x845648, 16,
     lhiver::digest_literal("b74484567394a8339120ee796138a222810f4ab2eb227ec537b0c4983e9b8df3"),
     Kind::reviewed_site}, // Factory RTTI hierarchy
    {0x845638, 12,
     lhiver::digest_literal("5aef05ff7b13eeb027fc586815d26812b02561843e7e9d39f37168127f7fa755"),
     Kind::reviewed_site}, // Factory RTTI base array
    {0x845620, 24,
     lhiver::digest_literal("b1093005cd7cf59be0bd4f5bef448ff9f4b28aef6fa827a660ffca881d8ee3a0"),
     Kind::reviewed_site}, // Factory RTTI BaseClassDescriptor
    {0x845608, 24,
     lhiver::digest_literal("3d4266bb54f8ad26bb40c6b465166ed7f8fd90e608f76a2217845c5bf76a769c"),
     Kind::reviewed_site}, // Factory RTTI BaseClassDescriptor
    {0x89020c, 30,
     lhiver::digest_literal("7fc52b5e246b6c38e9389341c9402b4acef4e915ba9d91db6235a6af1c078bec"),
     Kind::reviewed_site}, // Factory RTTI TypeDescriptor
    {0x844d88, 24,
     lhiver::digest_literal("4bfe063710d92f90654928eac0bea5551672e3d3ca65d03cfedc6e08d8921ec4"),
     Kind::reviewed_site}, // Factory RTTI BaseClassDescriptor
    {0x88dde0, 23,
     lhiver::digest_literal("bae58a617fe880b16f56502e551ec6da9c190a8c1752c8656fbe31c3adc12279"),
     Kind::reviewed_site}, // Factory RTTI TypeDescriptor
    {0x792730, 168,
     lhiver::digest_literal("dd4bbcdc3a6b74c52b07a1ca491d823116c28fbda54b2a089351631aa763cdaa"),
     Kind::reviewed_site}, // DoStack switch targets
    {0x7927d8, 246,
     lhiver::digest_literal("cba5662b224c38d434541be24c173a8fd94947b2bf8904368e6a007453c03a9d"),
     Kind::reviewed_site}, // DoStack opcode map
    {0x8c1ed0, 1,
     lhiver::digest_literal("6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d"),
     Kind::reviewed_site}, // ItemInit empty literal
}};
inline constexpr std::array<std::uint16_t, 2> free_reloc_0{{18, 24}};
inline constexpr std::array<std::uint16_t, 1> free_reloc_1{{10}};
inline constexpr std::array<zbug_end_compat::CodeRange, 2> free_code{{
    {0x1014e, 38,
     lhiver::digest_literal("8172c2285a94af709ff22bf060c06f02b141eeed742a107d5b5cf2950f38a1aa"),
     free_reloc_0.data(), free_reloc_0.size()},
    {0x3f290, 29,
     lhiver::digest_literal("c507b5d2d3e25fb4dcb299d96339f9001b173dcc23ccb763580567944b58b271"),
     free_reloc_1.data(), free_reloc_1.size()},
}};
inline constexpr std::array<alchemy_flask_runtime::StringContract, 3> texts{{
    {46463, 15,
     lhiver::digest_literal("66705d1af998a374712b62840b34f91d9f9f6139a69d945de881cc2f8c136862")},
    {46464, 21,
     lhiver::digest_literal("32aacfe6e628badd91cbfdf68bda88193c3fef2143150029af1b4e0c76fb7793")},
    {14210, 5,
     lhiver::digest_literal("1dcd75ced3627e99c880afe118bffedce818cc3f2a5cfd94b76b3f935a609130")},
}};
inline constexpr std::array<lhiver::SymbolBinding, 6> bindings{{
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 0x200000},
    {"VALUE_SWORDBLADEHOT", 16545, 2, 1, 1, -1, true, 8},
    {"MAT_METAL", 1262, 2, 1, 1, -1, true, 2},
    {"C_NPC.AIVAR", 1563, 2, 4, 100, 1537, false, 0, true, 0x288},
    {"AIV_INVINCIBLE", 11998, 2, 1, 1, -1, true, 4},
}};
inline constexpr auto uppercase_sha256 =
    lhiver::digest_literal("687637f6c132d1e844f4ee2be932eaca8fb168a0d04fed5384ba9615ead4e20f");
} // namespace ubc::cool_predebit::profile

#include "SpawnMenu.h"
#include "BarcodeInjector.h"
#include "SavePatcher.h"
#include "tip_engine/TextureTools.h"
#include <tip_engine/hooks.h>
#include "tip_engine/rex_macros.h"
#include "tip_engine/Log.h"
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <rex/runtime.h>
#include <rex/ppc/function.h>

// Cursor functions (declared in hooks.cpp via PPC_EXTERN_IMPORT)
extern "C" void rex_cursorMainGetWorkspace_821F6840(PPCContext& ctx, uint8_t* base);
extern "C" void rex_cursorMainGetCursor_822CC598(PPCContext& ctx, uint8_t* base);

// Wildcard barcode database
struct WildcardEntry {
    const char* name;
    const char* barcode;
};

static const WildcardEntry g_WildcardDB[] = {
    {"ArcticBunnycomb Wildcard", "BE81C35045862917 FB741940D504F384"},
    {"Arocknid Desert Wildcard1", "92FBDC82EB48F666 DA03DC53DB55AD05"},
    {"Arocknid Desert Wildcard2", "D1746B2A2CA6A3CF 93EE761DAE24CE4A"},
    {"Arocknid Wildcard1", "A3F06416B30B44B2 F3B4A85B766C8C81"},
    {"Arocknid Wildcard2", "E0CAC90F49DBF72E D172CDEBDB1AF0AF"},
    {"Arocknid Wildcard3", "9DEFFC82EB4DF6B6 87EE761DAAD4CA6A"},
    {"Badgesicle Wildcard1", "D96FE940FFA50445 E70E6983AAFFC646"},
    {"Badgesicle Wildcard2", "A2CAE4AE183BFCFA D172CDEBDB1AADAF"},
    {"Badgesicle Wildcard3", "FE4CA050FDF64947 D3D1D5C8CB48CE68"},
    {"Barkbark Wildcard1", "D9E5D834EC6BD2A4 B7EE761DAE26CEEA"},
    {"Barkbark Wildcard2", "84F1C4C47F67CF98 F3F7A85B773E8C81"},
    {"Barkbark Wildcard3", "D1603B2A2C7B53CF E0ED7F9F9724E4DE"},
    {"Bispotti Wildcard1", "E1FAE80F590BF72E D172CDEBD69EC9AF"},
    {"Bispotti Wildcard2", "D92FE150FD550465 B23560704B9493DB"},
    {"Bispotti Wildcard3", "9DEFDC82EA4DE246 9A632C53DB55AD85"},
    {"Bonboon Wildcard1", "D4F071E0DEBA23FD F397A85B77D38C81"},
    {"Bonboon Wildcard2", "CC74FB36ED4F82A4 EB7FC14B0965DD6C"},
    {"Bonboon Wildcard3", "84D1E4F01B669F98 F64C23F0785450B7"},
    {"Bunnycomb Wildcard1", "82092BF7FC45CCF9 E1F4D67C730E87B0"},
    {"Bunnycomb Wildcard3", "A24965B3FC45CCE9 F3F1D1E8C8A8C748"},
    {"BunnycombArctic Wildcard1", "82092BF7FC45CCF9 E1F4D67C730E87B0"},
    {"BunnycombArctic Wildcard2", "F9569C74269FC1F1 F7D5F5E8C85DC768"},
    {"BunnycombArctic Wildcard3", "A2CAE0BE599FBEFA E4A464339FBCD044"},
    {"Buzzenge Wildcard1", "F9769B3B269F8AF1 E1F4A953730E87B0"},
    {"Buzzenge Wildcard2", "DA66E050FDA72B45 FBAB60379BB8D244"},
    {"Buzzenge Wildcard3", "860C65B36C05CCE9 D3F1D1C8CB4DC768"},
    {"Buzzlegum Wildcard1", "E0FAC90F5C1BF72E D172CDEBDF9AAAAF"},
    {"Buzzlegum Wildcard2", "FE4CE15BDDF20947 E70E6983AAFADF06"},
    {"Buzzlegum Wildcard3", "B611F5A10D761EB3 F4E26E3AA3BE2BDD"},
    {"Camello Wildcard1", "84F1C1D41B669F98 964CD753F054F329"},
    {"Camello Wildcard2", "8360B0A47A2DE06B 8A63FC50DF51AD05"},
    {"Camello Wildcard3", "FBF071E02EBA2C4D BE6260C96E760A03"},
    {"Candary Wildcard1", "D1656B3A2D7653CF 8A236C53DB51AD85"},
    {"Candary Wildcard2", "FE4CA05BDDC60B07 D172CDEBD49E915F"},
    {"Candary Wildcard3", "960935F36C45CCE9 E1F4CA73330E87B0"},
    {"Cherrapin Wildcard1", "A2CAE4AE1D967CFA 8D2560704BC4918B"},
    {"Cherrapin Wildcard2", "F94F993427CF95B1 D7D1F5C8CBADE668"},
    {"Cherrapin Wildcard3", "E07B62A20300FF00 E0ED7F9F9E943FDE"},
    {"Chewnicorn Wildcard1", "E07B63920349FF80 964E275DF456F169"},
    {"Chewnicorn Wildcard2", "D974F834EC6FD094 A3EE761DEA26CA7A"},
    {"Chewnicorn Wildcard3", "D92F7042FFAE2B65 D172CDEBD69AF75F"},
    {"Chippopotamus Wildcard1", "E1EBC90F5CC8F72E D172CDEBD615B4AF"},
    {"Chippopotamus Wildcard2", "A2CAE5BD58966EFA 8D7560704F9F110B"},
    {"Chippopotamus Wildcard3", "FE4CA154D0D20A47 A70E6983A98ACF46"},
    {"Chocstrich Wildcard1", "D9EAD8C6EC4F82A4 A7EE761DAEDBCAFA"},
    {"Chocstrich Wildcard2", "8360B0A13ADDA87B CA236C50DB51AD05"},
    {"Chocstrich Wildcard3", "84D1C0C07F269F98 F3A6585B776E8C81"},
    {"Cinnamonkey Wildcard1", "B561E8A5FDFB08F3 436CB98BA1E55668"},
    {"Cinnamonkey Wildcard2", "A2CAE58C1D34FEFA D172CDEBD6CAEF5F"},
    {"Cinnamonkey Wildcard3", "E07B6E520323FF00 F397585B76278C81"},
    {"Cluckles Wildcard1", "E07B79920300FF80 924ED753F056F369"},
    {"Cluckles Wildcard2", "84F1C0D47B279F98 F3B6A85B767A8C81"},
    {"Cluckles Wildcard3", "960CC4B7FC05CCE9 D4E27A74A3BE2B2D"},
    {"Cocoadile Wildcard1", "D96FF842FDAA2445 8D3560704BC4112B"},
    {"Cocoadile Wildcard2", "C9AD2F460E4414DC F4E27E34A3BE0F0D"},
    {"Cocoadile Wildcard3", "A2CAE4AE58164CFA EBA466379F4D2244"},
    {"Crowla Wildcard1", "B61164A11D7617B3 ADB17665AA3A7742"},
    {"Crowla Wildcard2", "FE4CE158DDD20A47 D172CDEBDF9AA55F"},
    {"Crowla Wildcard3", "E07B7DA20318FF00 E0ED7F9F9A14EFDE"},
    {"Custacean Wildcard1", "C94D2F460C10162C F4E27E34A3BE2BDD"},
    {"Custacean Wildcard2", "D974FBC6CD6BD2A4 BA6240C36F561A03"},
    {"Custacean Wildcard3", "B61165A11D761EB3 ADB17665AA3A7742"},
    {"Doenut Wildcard1", "E1E8E90F5C36F72E E1F4A850730E87B0"},
    {"Doenut Wildcard2", "C25F2382EB48E2B6 F74C63E07B1450B7"},
    {"Doenut Wildcard3", "A3F3742FA30E46F2 B56260CC6EE61A03"},
    {"Dragumfly Wildcard1", "8360B0A13A02506B BE62B0CC6F960903"},
    {"Dragumfly Wildcard2", "84F5E1F03F269F98 F74321B1E05450B7"},
    {"Dragumfly Wildcard3", "E07B74520312FF00 E0ED7F9F9A848DDE"},
    {"Eaglair Wildcard1", "F956993B265FD1F1 D4E2FA75A3BEDFDD"},
    {"Eaglair Wildcard2", "D96F6150FEA52B45 D172CDEB8F1EBDAF"},
    {"Eaglair Wildcard3", "9D7FD382EB48F246 E0ED7F9F979480DE"},
    {"Elephanilla Wildcard1", "FE4CA1ABD2E64A07 E1F4A65FE30E87B0"},
    {"Elephanilla Wildcard2", "C9AD2F460E13CBDC E4A064369B48D044"},
    {"Elephanilla Wildcard3", "F95F9934265FD1F1 D4E2FA75A3BEDFDD"},
    {"Fizzlybear Wildcard1", "9D4BDC82EB48E246 E0ED7F9F97C4B2DE"},
    {"Fizzlybear Wildcard2", "B8F071E03EBA2DBD CB5D314B09670D6C"},
    {"Fizzlybear Wildcard3", "E0DBC90F4C00F72E E1F4F556330E87B0"},
    {"Flapyak Wildcard1", "C94D2F460D7196DC D172CDEBDB1E9A5F"},
    {"Flapyak Wildcard2", "F9269D34261F99F1 BD3560704B9F132B"},
    {"Flapyak Wildcard3", "F9269B34261F8AB1 BD2560704F3F932B"},
    {"Fourheads Wildcard1", "F9F071E0DEBA283D B562F0CC6E961A03"},
    {"Fourheads Wildcard2", "A362642FA30A46F2 C57D314B0965DD6C"},
    {"Fudgehog Wildcard1", "8360B0A02808783B C57FC14B0947026C"},
    {"Fudgehog Wildcard2", "C2AB6382EB48F6F6 DA03FDA1DF55AD05"},
    {"Fudgehog Wildcard3", "D974DBC4CC6BD0A4 BA6240C16F161A03"},
    {"Galagoogoo Wildcard1", "F96F9E3B267F81F1 A8B17667AA3B3742"},
    {"Galagoogoo Wildcard2", "C9BD2F461C519B2C BC6560704434112B"},
    {"Galagoogoo Wildcard3", "CC74F8C4ED6B82A4 A7EE761DA5D4CEAA"},
    {"Geckie Wildcard2", "84F5E4C01F27CF88 F381585B77358C81"},
    {"Geckie Wildcard3", "8360B0A53804A07B B3EE761DA7D6C75A"},
    {"Goobaa Wildcard1", "D92F6942FFA52445 D172CDEBDB1EFE5F"},
    {"Goobaa Wildcard2", "C9BD2F460D0E9FDC BC3560704B9F118B"},
    {"Goobaa Wildcard3", "FE4CE059F0E24907 D7D5D1E8CBA8EE48"},
    {"Hoghurt Wildcard1", "A2CAE4AF103BACFA ADE17657AA3E9742"},
    {"Hoghurt Wildcard2", "925F2382EB08E246 B76220CC6EA61903"},
    {"Hoghurt Wildcard3", "A0F3743FA30E46F2 E0ED7F9F9725B2DE"},
    {"Hootyfruity Wildcard1", "9D6F2982EB4DE2B6 F64C61E0E05410B7"},
    {"Hootyfruity Wildcard2", "C9AD2F460E083F2C 833560704F1B91DB"},
    {"Hootyfruity Wildcard3", "E07B69520312FF00 F3A6585B776E8C81"},
    {"Horstachio Wildcard1", "A2CAE4BF1894ACFA B70E6983A9BED646"},
    {"Horstachio Wildcard2", "F9469D3426AF95B1 F4E2EE7BA3BE248D"},
    {"Horstachio Wildcard3", "BD1161A1ADE61EB3 8C3560704F16110B"},
    {"Jameleon Wildcard1", "8360B0A0392DA03B A3EE761DAAD4CEFA"},
    {"Jameleon Wildcard2", "E07B79520300FF40 924ED753F056F369"},
    {"Jameleon Wildcard3", "860CDEF76C05CCE9 F4E27A7BA3BE26DD"},
    {"Jeli Wildcard1", "C94D2F461C71CBDC E1F4CF76E30E87B0"},
    {"Jeli Wildcard2", "B211FAA95CE61743 F7F1F5E8C848CF68"},
    {"Jeli Wildcard3", "8360B0A43C22E86B F382A85B76778C81"},
    {"Juicygoose Wildcard1", "E07B71120300FF80 924ED752F046F369"},
    {"Juicygoose Wildcard2", "84F5C0D07B279F98 F3B4585B767A8C81"},
    {"Juicygoose Wildcard3", "920CDAF76C05CCE9 ADF17643AA3E7742"},
    {"Kittyfloss Wildcard1", "A2CAE4AE181BACFA F70E6983AABFC646"},
    {"Kittyfloss Wildcard2", "E07B79120300FF00 924ED753F056F369"},
    {"Kittyfloss Wildcard3", "F976993426AF95B1 F4E27A7BA3BE26DD"},
    {"Lackatoad Wildcard1", "B21164A11DE60EB3 8C7560704B1491DB"},
    {"Lackatoad Wildcard2", "E07B6C120318FF00 A64ED75CF456F369"},
    {"Lackatoad Wildcard3", "B1F071E03EBA232D EA7DD14B096A0D6C"},
    {"Lemmoning Wildcard1", "B8F071E0DEBA2B7D C57DD14B09478D6C"},
    {"Lemmoning Wildcard2", "A3606426B30A44F2 BA62F0C16FC61A03"},
    {"Lemmoning Wildcard3", "BF5165A14DE617B3 F4A466369F482044"},
    {"Lickatoad Wildcard1", "E07B77120331BF80 A24E2712B446F329"},
    {"Lickatoad Wildcard2", "E4DBC80F4E96F72E F7D5F1C8C858EE48"},
    {"Limeoceros Wildcard1", "BF51F0A9ADEF0EB3 B36560704F9F118B"},
    {"Limeoceros Wildcard2", "FE4CA059F0E64947 D3D5D1E8CBA8CE68"},
    {"Limeoceros Wildcard3", "C9BD2F460F0ECF2C F4A864369BBDD144"},
    {"Macaraccoon Wildcard1", "E07B64441CBF88D0 A250355D065D3E4A"},
    {"Macaraccoon Wildcard2", "C94D2F460E5EC62C D4E6FA34A3BE0F8D"},
    {"Macaraccoon Wildcard3", "A2F37416A30B46F2 F3C4585B76F18C81"},
    {"Mallowolf Wildcard1", "FE4CA1EBD8E64947 EBA864369B49D044"},
    {"Mallowolf Wildcard2", "E1FBE90F4304F72E B32560704F16132B"},
    {"Mallowolf Wildcard3", "C9AD2F460E35CF2C E1F4A476E30E87B0"},
    {"Moojoo Wildcard1", "D1757B2A285B53CF F64C23A1745450B7"},
    {"Moojoo Wildcard2", "E07B6E920323FF40 924C275CB444F329"},
    {"Moojoo Wildcard3", "CC74D8C4EC4FD2A4 A7EE761DA5DFCAAA"},
    {"Moozipan Wildcard1", "D1F071E03EBA2E7D F3A7585B777C8C81"},
    {"Moozipan Wildcard2", "DA6FE842FDA72B65 FBA462379B482044"},
    {"Moozipan Wildcard3", "D1757B2A2C7E53CF DA236CA1DB51AD05"},
    {"Mothdrop Wildcard1", "F96F9E7B277F91F1 A8817647AA3BC742"},
    {"Mothdrop Wildcard2", "B64EEFF7FC45CCE9 D4A66E74A3BE04DD"},
    {"Mothdrop Wildcard3", "D92FF152FF5E0465 D172CDEBD49ECE5F"},
    {"Mousemallow Wildcard1", "B60C2BF76C45CCF9 E1F4F763730E87B0"},
    {"Mousemallow Wildcard2", "F9769C74279F95F1 D7D5F5C8C8BDC768"},
    {"Mousemallow Wildcard3", "81F271E03EBA225D F382585B766C8C81"},
    {"Newtgat Wildcard1", "80F071E0DEBA265D 924ED75CF0B4F369"},
    {"Newtgat Wildcard2", "FE4CE15AFDC60947 D172CDEBDFCEE0AF"},
    {"Newtgat Wildcard3", "D1716B2A2937A3CF CA636C53DB51AD05"},
    {"Parmadillo Wildcard1", "88F071E0DEBA2E1D A64ED753F0A4F369"},
    {"Parmadillo Wildcard2", "D1746B2A2C36A3CF DA23FC53DF51AD05"},
    {"Parmadillo Wildcard3", "A3606436F30F46F2 F3F6A85B777C8C81"},
    {"Parrybo Wildcard1", "E1D8E90F5920F72E D3D5D1E8CBBDCE48"},
    {"Parrybo Wildcard2", "FBF66DE4094B321D 06A308DC1484A546"},
    {"Parrybo Wildcard3", "DA2FE050FEAA0B45 F70E698355BBC6B6"},
    {"Peckanmix Wildcard1", "B9F071E0DEBA275D BE62F0C76FA60A03"},
    {"Peckanmix Wildcard2", "B25165A99DE607B3 FBA460369B482144"},
    {"Peckanmix Wildcard3", "D161782A293453CF F64C61E0E05410B7"},
    {"Pengum Wildcard1", "A2CAE4AE1D16ECFA 8D6560704BC491DB"},
    {"Pengum Wildcard2", "D92FE042FDA50445 EBA462379FBDD344"},
    {"Pengum Wildcard3", "F966993427CF95B1 D7D1F5C8CBBDE768"},
    {"Pieena Wildcard1", "A3F06426B30A44B2 BA62F0C16FC61A03"},
    {"Pieena Wildcard2", "B0F071E03EBA2B6D C57DC14B09470D6C"},
    {"Pieena Wildcard3", "E0F9E90F4CC4F72E D7F5D5E8CB5DEE48"},
    {"Pigxie Wildcard1", "E1EBC90F5C9DF72E B70E6983A9DACF06"},
    {"Pigxie Wildcard2", "BD11F1A18D761743 F4E2FE34A3BE0F8D"},
    {"Pigxie Wildcard3", "F94F9F34261F99F1 E1F4B05A730E87B0"},
    {"Polollybear Wildcard1", "A3F3640FE30F45B2 F397A85B77D98C81"},
    {"Polollybear Wildcard2", "D5F071E02EBA236D A20C275CF4A6F329"},
    {"Polollybear Wildcard3", "CDAF6182EB4DE2F6 9A236DA1D855AD05"},
    {"Ponocky Wildcard1", "B25160A19DE60743 FBAB62369BB82344"},
    {"Ponocky Wildcard2", "FE4CA151F8D24A07 D3F1D1C8CBBDEF48"},
    {"Ponocky Wildcard3", "F96ED21027B73AD1 A2A2694D4C4B9DB8"},
    {"Pretztail Wildcard1", "FE4EE0ABD2E20B47 E78E69D3A1FEDF06"},
    {"Pretztail Wildcard2", "F94F9E74263F91B1 D3D5F5E8C858E668"},
    {"Pretztail Wildcard3", "D96F6142FD5E0465 E4A066379B4CD044"},
    {"Profitamole Wildcard1", "FE4CE0AAF8F20A47 D172CDEBDBC5EBAF"},
    {"Profitamole Wildcard2", "C2ABD982EB4DE666 93EE761DAE06CAAA"},
    {"Profitamole Wildcard3", "C9AD2F460D0B942C F4E66E35A3BE240D"},
    {"Pudgeon Wildcard1", "C2FBD982EB4DE6F6 CA23FCA3D855AD05"},
    {"Pudgeon Wildcard2", "8BF071E03EBA273D F3F7A85B77298C81"},
    {"Quackberry Wildcard1", "A2CAE48E18CF5CFA E4A060379FB82044"},
    {"Quackberry Wildcard2", "9DABF182EB48F646 DA432D53DD55AD05"},
    {"Quackberry Wildcard3", "D96F7140FDAA2B65 B27560704B14910B"},
    {"Raisant Wildcard1", "9DABD982EB48E646 93EE761DAA86CEBA"},
    {"Raisant Wildcard2", "A2CAE48E183F6EFA EBA060379F482044"},
    {"Raisant Wildcard3", "864C36F76C05CCF9 D7F5F5C8CB48C748"},
    {"Rashberry Wildcard1", "FE4CE0A5F8D20A07 B70E69835ABECF86"},
    {"Rashberry Wildcard2", "D92FE052FDAA2445 8D6560704BC491DB"},
    {"Rashberry Wildcard3", "E0C8C90F493DF72E D172CDEBDB35E55F"},
    {"Reddhott Wildcard1", "84F5E1D41B6F9F98 F3C4585B76E78C81"},
    {"Reddhott Wildcard2", "8360B0A46A02E06B A7EE761DA50BCE4A"},
    {"Reddhott Wildcard3", "E07B75A2032FFF00 960CD752B044F329"},
    {"Roario Wildcard1", "FE4EE0A5DDE64947 F7D1F5E8CBB8CF48"},
    {"Roario Wildcard2", "C9AD2F461D59CF2C EBA064329FBD2344"},
    {"Roario Wildcard3", "A2F36416E30E44F2 EB5DD14B096E2D6C"},
    {"Robean Wildcard1", "C94D2F460F1CC82C B32560704F94138B"},
    {"Robean Wildcard2", "F8F271E03EBA35AD BE62D0DA6F960903"},
    {"Robean Wildcard3", "BF31F5A9ADEF07B3 E4A064369B4CD144"},
    {"S'morepion Wildcard1", "BF11F4A11DE60EB3 E4A062369F48D144"},
    {"S'morepion Wildcard2", "E07B67520318FF00 F3F5585B77678C81"},
    {"S'morepion Wildcard3", "8360B0A17980A06B CA03DCA2DB51AD05"},
    {"Salamango Wildcard1", "A2F0642FE30B44B2 A24CD752B4B6F369"},
    {"Salamango Wildcard2", "A2F1742FA30B41F2 F3A5585B74C38C81"},
    {"Salamango Wildcard3", "D1F071E02EBA2AED F3A5585B77718C81"},
    {"Sarsgorilla Wildcard1", "F956993B26EFD1F1 ADF17663AA3D5742"},
    {"Sarsgorilla Wildcard2", "8209D7F7FC05CCE9 D4E2EA7BA3BEDFDD"},
    {"Sarsgorilla Wildcard3", "FC658194680FB4DB D3AD9118A2440A1F"},
    {"Shellybean Wildcard1", "BBF271E0DEBA0C7D CA7DD14B096E826C"},
    {"Shellybean Wildcard2", "A1626426F30E4562 BA62F0C16E760A03"},
    {"Shellybean Wildcard3", "D165683A233BA3CF F74C63E1E45010B7"},
    {"Sherbat Wildcard1", "864CC4B7FC05CCF9 D4E26A7AA3BE2B2D"},
    {"Sherbat Wildcard2", "8360B0A03922E87B DA632C52DF51AD05"},
    {"Sherbat Wildcard3", "A2CAE48C583B4EFA D172CDEBDFCEACAF"},
    {"Smelba Wildcard1", "B61164A95D7F1E43 ADF17645AA3F5742"},
    {"Smelba Wildcard2", "D4F071E03EBA2D0D 960C2753F0A6F329"},
    {"Smelba Wildcard3", "846662E42C096F0D 66E66F8DEE054970"},
    {"Sparrowmint Wildcard1", "D165783A28E753CF 9A036C51DD51AD85"},
    {"Sparrowmint Wildcard2", "A3F2741FB30B4462 924ED71DF0A4F329"},
    {"Sparrowmint Wildcard3", "9DEFD382EA4DF2B6 87EE761DA5D4C56A"},
    {"Squazzil Wildcard1", "C94D2F460C0C34DC D4E6EA3AA3BE2B2D"},
    {"Squazzil Wildcard2", "E0F9E90F4ADDF72E D172CDEBDFCEF1AF"},
    {"Squazzil Wildcard3", "D1716B2A29E653CF CA436C53DF51AD05"},
    {"Swanana Wildcard1", "84D5C1D43F269F98 A24CD75DF054F369"},
    {"Swanana Wildcard2", "D5A6389E81BE2E5D 7412DB0294E720FC"},
    {"Swanana Wildcard3", "E07B61A20312FF00 F3A4A85B776E8C81"},
    {"Sweetle Wildcard1", "FE4CE0EBDDF24807 E1F4F754730E87B0"},
    {"Sweetle Wildcard2", "E0DAC80F4316F72E D7D5F5C8CB48EE68"},
    {"Sweetle Wildcard3", "C9AD2F460D309DDC EBA062369FBDD044"},
    {"Sweetooth Wildcard1", "F966993B261F91F1 D7D1F5E8CB48E668"},
    {"Sweetooth Wildcard2", "D96FF152FDA52445 BD2560704BC4112B"},
    {"Sweetooth Wildcard3", "864E2AF7FC05CCE9 E1F4FF55E30E87B0"},
    {"Syrupent Wildcard1", "84F5C1D45B279F88 F380585B767A8C81"},
    {"Syrupent Wildcard2", "8360B0A4390FE86B 8A436C52DF55AD85"},
    {"Syrupent Wildcard3", "E07B71920320BF00 924ED712F046F329"},
    {"Syrupent Desert Wildcard1", "DA6FE940FF572445 F70E69D3AADFC6B6"},
    {"Syrupent Desert Wildcard2", "F9769E74265F81B1 D4A26A7BA3BE0BDD"},
    {"Syrupent Desert Wildcard3", "D96FF140FF572B65 D172CDEBD69E9D5F"},
    {"Taffly Wildcard1", "FE4CA1ABDDF20B47 E70E69D3AADADF06"},
    {"Taffly Wildcard2", "C9AD2F460E10902C A8A17665AA3A9742"},
    {"Taffly Wildcard3", "A2606426F30F4462 F3B3A85B77738C81"},
    {"Tartridge Wildcard1", "BF31FBA15DEF07B3 E4A066329FBCD144"},
    {"Tartridge Wildcard2", "FE4EE059D2E24B47 E1F4D66B730E87B0"},
    {"Tartridge Wildcard3", "C9BD2F461D1C982C BC6560704494138B"},
    {"Tigermisu Wildcard1", "84F071E0DEBA353D A64E275DF4B4F169"},
    {"Tigermisu Wildcard2", "D134682A29CEA3CF CA03FCA3D851AC05"},
    {"Tigermisu Wildcard3", "BF1169A11D7F0E43 AD817651AA3C7742"},
    {"Twingersnap Wildcard1", "B611F4A11DEF1E43 EBA062369F4DD244"},
    {"Twingersnap Wildcard2", "A2F07426B30E44F2 CB7DC14B09478D6C"},
    {"Twingersnap Wildcard3", "CD7FDC82EB48E646 F74C23A0705050B7"},
    {"Vulchurro Wildcard1", "CCEBDB36C84BD2A4 B74C21F0E81410B7"},
    {"Vulchurro Wildcard2", "8360B0A46F22507B E0ED7F9F972ABDDE"},
    {"Vulchurro Wildcard3", "C97AFB36E86B82A4 B74C21B17B5050B7"},
    {"Walrusk Wildcard1", "8360B0A43F22783B E0ED7F9F978AB8DE"},
    {"Walrusk Wildcard2", "A060743FE30F44F2 DA232C51DF55AD85"},
    {"Walrusk Wildcard3", "CC7BDBC4E94BD2A4 B74C21F0E45410B7"},
    {"Whirlm Wildcard1", "80F1E1D43B279F88 924ED753B054F369"},
    {"Whirlm Wildcard2", "E07B5AA20300BF00 F3B6A85B76778C81"},
    {"Whirlm Wildcard3", "8360B0A03C26E06B DA632C52DF51AD05"},
    {"WhiteFlutterscotch Wildcard1", "F93F9E7B263F91F1 E1F4B657C30E87B0"},
    {"WhiteFlutterscotch Wildcard2", "896FF150FD5E0465 B27560704F94112B"},
    {"WhiteFlutterscotch Wildcard3", "E4EBC80F4308F72E D172CDEB8F15A9AF"},
    {"Zumbug Wildcard1", "C94D2F460F2594DC B78E69D3AADEDF86"},
    {"Zumbug Wildcard2", "E1D8E90F471DF72E D4A26E34A3BE040D"},
    {"Zumbug Wildcard3", "864E75F36C05CCC9 BD7560704F36932B"},
};
static constexpr int g_WildcardDBSize = sizeof(g_WildcardDB) / sizeof(g_WildcardDB[0]);

void SpawnMenuDialog::OnDraw(ImGuiIO& io) {
    // Toggle menu with F8
    if (ImGui::IsKeyPressed(ImGuiKey_F8, false)) {
        g_SpawnMenuOpen = !g_SpawnMenuOpen;
    }

    if (!g_SpawnMenuOpen) return;

    ImGui::SetNextWindowSize(ImVec2(480.0f, 750.0f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Pinata Vision Spawner", &g_SpawnMenuOpen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    // Status message
    if (g_SpawnRequest.pending) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Spawn pending...");
    }

    ImGui::Separator();

    // Search bar
    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer));

    // Category filter
    ImGui::Text("Category:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##category", GetCategoryName(categoryFilter))) {
        for (int i = 0; i <= 7; i++) {
            if (ImGui::Selectable(GetCategoryName(i), categoryFilter == i)) {
                categoryFilter = i;
                selectedIndex = -1;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    // Variant/Wildcard control
    static int variantIndex = -1;
    ImGui::Text("Variant:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##variant", &variantIndex, -1, 20, variantIndex == -1 ? "Default" : "%d");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Set color variant. -1=Default, 0+=Wildcard variants.\nTry different values to discover hidden colors!");
    }

    // Wildcard Trait (separate from color variant)
    static int wildcardTrait = 0;
    ImGui::Text("Wildcard:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##wildcard", &wildcardTrait, 0, 3, wildcardTrait == 0 ? "None" : "Trait %d");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Wildcard body traits (fangs, mane, etc.)\n0=None, 1-3=Different wildcard features");
    }
    static bool wildcardAsEgg = true;
    if (wildcardTrait > 0) {
        ImGui::Checkbox("Spawn as Egg (full wildcard)", &wildcardAsEgg);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Egg: hatches as full wildcard with star + 10x value\nUnchecked: spawns adult with wildcard model only (no star)");
        }
    }

    // Dino Color (Choclodocus/Dragonache special color)
    static int dinoColor = -1;
    static const char* dinoColorNames[] = {"Default", "Blue", "Green", "Red", "Elite Neon"};
    ImGui::Text("Dino Color:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##dinocolor", &dinoColor, -1, 3,
        (dinoColor >= 0 && dinoColor <= 3) ? dinoColorNames[dinoColor + 1] : dinoColorNames[0]);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Choclodocus/Dragonache color.\n-1=Default, 0=Blue, 1=Green, 2=Red, 3=Elite Neon");
    }

    ImGui::Separator();

    // Item list
    float listHeight = ImGui::GetContentRegionAvail().y - 40.0f;
    if (ImGui::BeginChild("##itemlist", ImVec2(0, listHeight), true)) {
        int visibleIdx = 0;
        for (size_t i = 0; i < g_PinataIDs.size(); i++) {
            const auto& item = g_PinataIDs[i];

            // Category filter
            if (categoryFilter != 0 && GetCategory(item) != categoryFilter)
                continue;

            // Search filter
            if (!MatchesSearch(item.Name, searchBuffer))
                continue;

            // Color by category
            int cat = GetCategory(item);
            ImVec4 color;
            switch (cat) {
                case 1: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; // Animals - red
                case 2: color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f); break; // Eggs - orange
                case 3: color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); break; // Seeds - green
                case 4: color = ImVec4(0.4f, 0.6f, 1.0f, 1.0f); break; // Homes - blue
                case 5: color = ImVec4(0.8f, 0.4f, 1.0f, 1.0f); break; // Props - purple
                case 6: color = ImVec4(0.4f, 1.0f, 0.8f, 1.0f); break; // Trees - teal
                default: color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break; // Other - gray
            }

            char label[256];
            snprintf(label, sizeof(label), "[%u] %s", item.ID, item.Name);

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            bool selected = (selectedIndex == static_cast<int>(i));
            if (ImGui::Selectable(label, selected)) {
                selectedIndex = static_cast<int>(i);
            }
            ImGui::PopStyleColor();

            visibleIdx++;
        }
    }
    ImGui::EndChild();

    // Spawn button
    bool canSpawn = (selectedIndex >= 0 && selectedIndex < static_cast<int>(g_PinataIDs.size())
                     && !g_SpawnRequest.pending);

    if (!canSpawn) ImGui::BeginDisabled();

    if (ImGui::Button("Spawn Selected", ImVec2(-1, 0))) {
        g_SpawnRequest.tagID = g_PinataIDs[selectedIndex].ID;
        g_SpawnRequest.variantIndex = variantIndex;
        g_SpawnRequest.wildcardTrait = wildcardTrait;
        g_SpawnRequest.wildcardAsEgg = wildcardAsEgg;
        g_SpawnRequest.dinoColor = dinoColor;
        g_SpawnRequest.pending = true;
        Log("Spawn requested: " + std::string(g_PinataIDs[selectedIndex].Name)
            + (variantIndex >= 0 ? " (variant " + std::to_string(variantIndex) + ")" : ""), 3);
    }

    if (!canSpawn) ImGui::EndDisabled();

    // === Target Entity Selection ===
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "Target Entity:");
    static char entityAddrHex[20] = "";
    ImGui::SetNextItemWidth(150);
    ImGui::InputTextWithHint("##entityaddr", "hex addr (e.g. 4678D620)", entityAddrHex, sizeof(entityAddrHex));
    ImGui::SameLine();
    if (ImGui::Button("Set Target")) {
        uint32_t addr = (uint32_t)strtoul(entityAddrHex, nullptr, 16);
        if (addr > 0x40000000) {
            g_LastSpawnedEntity = addr;
            Log("Target entity set to 0x" + std::to_string(addr), 3);
        }
    }

    // Find existing garden entity by tag ID
    static int findTagID = 135; // default: Whirlm (worm=135)
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("Tag ID", &findTagID);
    ImGui::SameLine();
    if (ImGui::Button("Find in Garden")) {
        g_FindEntity.tagID = static_cast<uint32_t>(findTagID);
        g_FindEntity.pending = true;
        Log("Finding tag " + std::to_string(findTagID) + " in garden...", 3);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Uses game's entity finder (sub_82546C50) to locate\na pinata by species tag ID in the current garden.\nSets it as the target entity for dumping.");
    }

    if (g_LastSpawnedEntity != 0) {
        ImGui::Text("Current target: 0x%08X", g_LastSpawnedEntity);
    }

    // Dump Model button — exports last spawned entity's model data
    if (g_LastSpawnedEntity != 0) {
        if (ImGui::Button("Dump Model (last spawn)", ImVec2(-1, 0))) {
            g_ModelDumpRequested = true;
            Log("Model dump requested for entity 0x" + std::to_string(g_LastSpawnedEntity), 3);
        }
    }

    // Apply to last spawned entity (must be mature/resident first!)
    if (g_LastSpawnedEntity != 0) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Apply to Last Spawned (wait for maturity!):");

        if (variantIndex > 0) {
            if (ImGui::Button("Apply Color Change", ImVec2(-1, 0))) {
                g_DeferredVariantChange.entity = g_LastSpawnedEntity;
                g_DeferredVariantChange.variantIndex = variantIndex;
                g_DeferredVariantChange.isTrick = false;
                g_DeferredVariantChange.pending = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Triggers color variant change (like eating a turnip)\nSets entity+3636 and state 1093 for sparkle+swap");
            }
        }

        if (variantIndex > 0) {
            if (ImGui::Button("Apply Trick", ImVec2(-1, 0))) {
                g_DeferredVariantChange.entity = g_LastSpawnedEntity;
                g_DeferredVariantChange.variantIndex = variantIndex;
                g_DeferredVariantChange.isTrick = true;
                g_DeferredVariantChange.pending = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Triggers trick animation (like eating a buttercup)\n1=Trick1, 2=Trick2");
            }
        }

    }

    // === Color Tinting Experiment ===
    if (g_LastSpawnedEntity != 0) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 1.0f, 1.0f), "Color Tint (scenegraphInst experiment):");

        static float tintColor[4] = {1.0f, 0.0f, 0.0f, 1.0f}; // RGBA default red
        ImGui::ColorEdit4("Tint Color", tintColor);

        uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();
        uint32_t glModel = std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + 0x110));
        bool validModel = (glModel >= 0x40000000 && glModel < 0x50000000);

        if (validModel) {
            uint32_t sgInstBase = glModel + 0x138; // sgInst embedded at glModel+312

            // Dump sgInst state before any tint operation
            auto dumpSgInst = [&](const char* action) {
                std::ofstream dump("C:/Users/Administrator/Downloads/tint_debug.txt", std::ios::app);
                dump << std::endl << "=== TINT: " << action << " ===" << std::endl;
                dump << "entity=0x" << std::hex << g_LastSpawnedEntity << std::endl;
                dump << "glModel=0x" << glModel << std::endl;
                dump << "sgInstBase=0x" << sgInstBase << std::endl;
                dump << "solidColourEnabled=" << std::dec << (int)*(mb + sgInstBase + 0xFD) << std::endl;
                dump << "solidColour=0x" << std::hex << std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x100)) << std::endl;
                dump << "ambientOverride=0x" << std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x104)) << std::endl;
                dump << "isAmbientOverrideEnabled=" << std::dec << (int)std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x108)) << std::endl;
                dump << "textureBlendVal=" << *(float*)(mb + sgInstBase + 0x110) << std::endl;
                dump << "colourDisplacementTable=0x" << std::hex << std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x128)) << std::endl;
                dump << "colourDisplacementTableSize=" << std::dec << (int)std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x12C)) << std::endl;
                // Dump raw sgInst bytes around the color region (0xF0-0x130)
                // Verify sgInst base: +0x000 should be dbScenegraph_s* (valid heap ptr)
                uint32_t sgPtr = std::byteswap(*(uint32_t*)(mb + sgInstBase));
                dump << "sgInst+0x000 (sg ptr)=0x" << std::hex << sgPtr;
                if (sgPtr >= 0x40000000 && sgPtr < 0x50000000) dump << " [VALID heap]";
                else dump << " [INVALID — offset may be wrong!]";
                dump << std::endl;
                dump << "sgInst+0x004 (nodeCount)=" << std::dec << (int)std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x004)) << std::endl;
                dump << "sgInst+0x0F4 (numJoints)=" << (int)std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x0F4)) << std::endl;

                dump << "raw sgInst+0x00..0x20:" << std::endl;
                for (int off = 0x00; off < 0x20; off += 16) {
                    dump << "  +" << std::hex << off << ": ";
                    for (int j = 0; j < 16; j++) {
                        char buf[4]; snprintf(buf, 4, "%02X ", *(mb + sgInstBase + off + j));
                        dump << buf;
                    }
                    dump << std::endl;
                }
                dump << "raw sgInst+0xF0..0x140:" << std::endl;
                for (int off = 0xF0; off < 0x140; off += 16) {
                    dump << "  +" << std::hex << off << ": ";
                    for (int j = 0; j < 16; j++) {
                        char buf[4]; snprintf(buf, 4, "%02X ", *(mb + sgInstBase + off + j));
                        dump << buf;
                    }
                    dump << std::endl;
                }
                dump << "tintColor: R=" << tintColor[0] << " G=" << tintColor[1] << " B=" << tintColor[2] << " A=" << tintColor[3] << std::endl;
                dump.close();
            };

            if (ImGui::Button("Dump Full sgInst (SAFE — read only)", ImVec2(-1, 0))) {
                std::ofstream dump("C:/Users/Administrator/Downloads/sginst_full_dump.txt", std::ios::app);
                dump << std::endl << "=== FULL sgInst DUMP ===" << std::endl;
                dump << "entity=0x" << std::hex << g_LastSpawnedEntity << std::endl;
                dump << "glModel=0x" << glModel << " (entity+0x110)" << std::endl;
                dump << "sgInstBase=0x" << sgInstBase << " (glModel+0x138)" << std::endl;

                // Dump all 0x194 bytes with annotations
                for (int off = 0; off < 0x194; off += 16) {
                    dump << std::hex << "  +" << std::setw(3) << std::setfill('0') << off << ": ";
                    // Hex bytes
                    for (int j = 0; j < 16 && (off + j) < 0x194; j++) {
                        char buf[4]; snprintf(buf, 4, "%02X ", *(mb + sgInstBase + off + j));
                        dump << buf;
                    }
                    dump << " | ";
                    // As big-endian uint32s
                    for (int j = 0; j < 16 && (off + j + 3) < 0x194; j += 4) {
                        uint32_t val = std::byteswap(*(uint32_t*)(mb + sgInstBase + off + j));
                        if (val >= 0x40000000 && val < 0x50000000)
                            dump << "[ptr] ";
                        else if (val >= 0x80000000 && val < 0x90000000)
                            dump << "[code] ";
                        else if (val == 0)
                            dump << "[0] ";
                        else if (val <= 200)
                            dump << "[" << std::dec << val << "] " << std::hex;
                        else {
                            float f; memcpy(&f, &val, 4);
                            if (f > 0.001f && f < 1000.0f && !std::isnan(f))
                                dump << "[f=" << std::fixed << std::setprecision(3) << f << "] " << std::hex;
                            else
                                dump << "[0x" << std::hex << val << "] ";
                        }
                    }
                    dump << std::endl;
                }
                dump.close();
                Log("sgInst dump written to sginst_full_dump.txt", 3);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Dumps the full 0x194-byte sgInst struct to file.\nSafe — read-only, no writes to game memory.");
            }

            // Debug info
            uint8_t solidEnabled = *(mb + sgInstBase + 0xFD);
            uint32_t solidCol = std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x100));
            uint32_t ambientEnabled = std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x108));
            uint32_t ambientCol = std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x104));
            uint32_t cdtPtr = std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x128));
            int cdtSize = (int)std::byteswap(*(uint32_t*)(mb + sgInstBase + 0x12C));
            ImGui::Text("sgInst @ 0x%08X", sgInstBase);
            ImGui::Text("solidColour: enabled=%d rgba=0x%08X", solidEnabled, solidCol);
            ImGui::Text("ambientOverride: enabled=%d rgba=0x%08X", ambientEnabled, ambientCol);
            ImGui::Text("colourDisplacementTable: ptr=0x%08X size=%d", cdtPtr, cdtSize);
        } else {
            ImGui::TextDisabled("No valid glModel on last spawned entity");
        }
    }

    // === Dragonache Editor ===
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Dragonache Editor:");

    static int dragonColor = 0;
    static int dragonTeeth = 0;
    static int dragonMane = 0;
    static int dragonWings = 0;
    static int dragonTail = 0;
    static int dragonRidges = 0;

    static const char* colorNames[] = {"Dirt (Brown)", "Gold", "Grass (Green)", "Water (Blue)", "Snow (White)", "Sand (Red)"};
    ImGui::Combo("Color##dragon", &dragonColor, colorNames, 6);
    ImGui::SliderInt("Teeth##dragon", &dragonTeeth, 0, 3);
    ImGui::SliderInt("Mane##dragon", &dragonMane, 0, 3);
    ImGui::SliderInt("Wings##dragon", &dragonWings, 0, 3);
    ImGui::SliderInt("Tail##dragon", &dragonTail, 0, 3);
    ImGui::SliderInt("Ridges##dragon", &dragonRidges, 0, 3);

    if (ImGui::Button("Spawn Custom Dragonache", ImVec2(-1, 0))) {
        g_SpawnRequest.tagID = 32; // Dragonache (adult)
        g_SpawnRequest.variantIndex = -1;
        g_SpawnRequest.wildcardTrait = 0;
        g_SpawnRequest.dinoColor = -1;
        g_SpawnRequest.pending = true;

        // Queue deferred body part + color customization
        g_DeferredDragonache.color = dragonColor;
        g_DeferredDragonache.teeth = dragonTeeth;
        g_DeferredDragonache.mane = dragonMane;
        g_DeferredDragonache.wings = dragonWings;
        g_DeferredDragonache.tail = dragonTail;
        g_DeferredDragonache.ridges = dragonRidges;
        g_DeferredDragonache.framesRemaining = 10;
        g_DeferredDragonache.pending = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Spawns an adult Dragonache with custom body parts + color.\n20,000+ combinations!");
    }

    // Apply body parts to existing adult Dragonache
    if (g_LastSpawnedEntity != 0) {
        if (ImGui::Button("Apply Parts to Last Spawned##dragon", ImVec2(-1, 0))) {
            g_DeferredDragonache.entity = g_LastSpawnedEntity;
            g_DeferredDragonache.color = dragonColor;
            g_DeferredDragonache.teeth = dragonTeeth;
            g_DeferredDragonache.mane = dragonMane;
            g_DeferredDragonache.wings = dragonWings;
            g_DeferredDragonache.tail = dragonTail;
            g_DeferredDragonache.ridges = dragonRidges;
            g_DeferredDragonache.framesRemaining = 1;
            g_DeferredDragonache.pending = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Applies body part settings to an existing Dragonache.\nDoes not change color — that's set by terrain at hatch.");
        }
    }

    // === Wildcard Breeding Override ===
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 1.0f, 1.0f), "Wildcard Breeding Override:");
    ImGui::Checkbox("Force Wildcard on Hatch", &g_ForceWildcard);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When enabled, ALL hatched eggs will be wildcards!\nHooks the egg hatching probability function (sub_824144F8)");
    }
    if (g_ForceWildcard) {
        const char* traitNames[] = { "Trait 1", "Trait 2", "Trait 3" };
        ImGui::Combo("Wildcard Trait", &g_ForcedWildcardTrait, traitNames, 3);
    }

    // Auto Third Wildcard — breed two wildcards to get the missing third
    ImGui::Checkbox("Auto Third Wildcard", &g_AutoThirdWildcard);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When enabled, if you have 2 of 3 wildcard traits,\nthe next hatch automatically gets the missing third.\nCheck which traits you already have below.\n\nDesigned for single-player since trading servers are shut down.");
    }
    if (g_AutoThirdWildcard || g_ForceWildcard) {
        ImGui::Text("Traits owned:");
        ImGui::SameLine();
        ImGui::Checkbox("1##t1", &g_HasTrait[0]);
        ImGui::SameLine();
        ImGui::Checkbox("2##t2", &g_HasTrait[1]);
        ImGui::SameLine();
        ImGui::Checkbox("3##t3", &g_HasTrait[2]);

        int haveCount = (g_HasTrait[0] ? 1 : 0) + (g_HasTrait[1] ? 1 : 0) + (g_HasTrait[2] ? 1 : 0);
        if (haveCount >= 2) {
            int missing = -1;
            for (int i = 0; i < 3; i++) if (!g_HasTrait[i]) missing = i;
            if (missing >= 0)
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Next hatch will be Trait %d!", missing + 1);
            else
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "All traits collected!");
        } else {
            ImGui::TextDisabled("Check 2 traits to auto-breed the 3rd");
        }
    }

    // === Piñata Vision Barcode Injection ===
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Pinata Vision Barcode Inject:");

    static char barcodeHex[256] = "";  // enough for 4-row barcodes (Halo crossovers etc)

    // Preset buttons for key barcodes (no manual typing needed!)
    if (ImGui::Button("Elite Neon")) { strcpy(barcodeHex, "BF619A786BD25C9B"); }
    ImGui::SameLine();
    if (ImGui::Button("Dino Blue")) { strcpy(barcodeHex, "FD6198786BD25C9B"); }
    ImGui::SameLine();
    if (ImGui::Button("Dino Green")) { strcpy(barcodeHex, "8683F3F160A87698"); }
    ImGui::SameLine();
    if (ImGui::Button("Dino Red")) { strcpy(barcodeHex, "E0206B2A1A0EFE80"); }

    if (ImGui::Button("Dino Egg")) { strcpy(barcodeHex, "96FEF696AB02C4A6"); }
    ImGui::SameLine();
    if (ImGui::Button("Dragonache Egg")) { strcpy(barcodeHex, "F1706B687562A38F"); }
    ImGui::SameLine();
    if (ImGui::Button("Place Dino")) { strcpy(barcodeHex, "F1706B7B6D69A38F"); }
    ImGui::SameLine();
    if (ImGui::Button("Dino Journal")) { strcpy(barcodeHex, "D6727936AF6BF6B4"); }

    if (ImGui::Button("Whirlm V1")) { strcpy(barcodeHex, "E0F9EA0E1340F72E F029485B0255BC81"); }
    ImGui::SameLine();
    if (ImGui::Button("Whirlm V2")) { strcpy(barcodeHex, "A2CAE48E143A3CFA DBA2FC6A9CE00FDD"); }
    ImGui::SameLine();
    if (ImGui::Button("Whirlm V3")) { strcpy(barcodeHex, "E07B5AA20300DAE0 C47DC8832365804C"); }

    if (ImGui::Button("Amber Gem")) { strcpy(barcodeHex, "CB76D154B86F82E4"); }
    ImGui::SameLine();
    if (ImGui::Button("Wishing Well")) { strcpy(barcodeHex, "F1706A3BBC28538F"); }

    // Dino Bones — separate items (from barcode database)
    if (ImGui::Button("Red Bone")) { strcpy(barcodeHex, "96FEF69EAB02C4A6"); }
    ImGui::SameLine();
    if (ImGui::Button("Green Bone")) { strcpy(barcodeHex, "F1706A68E463A38F"); }
    ImGui::SameLine();
    if (ImGui::Button("Blue Bone")) { strcpy(barcodeHex, "96EAF69EAB02C4A6"); }

    // Halo crossover cards
    if (ImGui::Button("Master Chief")) { strcpy(barcodeHex, "E4E8C94C0925903E C0DD1418CE24FB9F C8401959DB55BB85 D56CD380785B7F87"); }
    ImGui::SameLine();
    if (ImGui::Button("Blue Spartan")) { strcpy(barcodeHex, "E4E8C94C0925903E 9679D90C29F1ACD6 F9CD096BCEB9D6CD B562FE8064E8C687"); }
    ImGui::SameLine();
    if (ImGui::Button("Cortana")) { strcpy(barcodeHex, "C1E2FE76DDBE1214 850D69EA6C110C01 B78F4AA0450FD706"); }
    ImGui::SameLine();
    if (ImGui::Button("Sgt Johnson")) { strcpy(barcodeHex, "BF6194A46E237E1B A31B39254850B359 41E178509C647D47"); }

    ImGui::Separator();
    ImGui::Text("Wildcard Barcodes (%d):", g_WildcardDBSize);
    static char wildcardFilter[64] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##wcfilter", "Search species...", wildcardFilter, sizeof(wildcardFilter));

    // Filtered list in a scrollable child
    ImGui::BeginChild("##wclist", ImVec2(-1, 120), true);
    std::string filterLower = wildcardFilter;
    std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
        [](unsigned char c){ return std::tolower(c); });
    for (int i = 0; i < g_WildcardDBSize; i++) {
        std::string nameLower = g_WildcardDB[i].name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
            [](unsigned char c){ return std::tolower(c); });
        if (filterLower.empty() || nameLower.find(filterLower) != std::string::npos) {
            if (ImGui::Selectable(g_WildcardDB[i].name)) {
                strcpy(barcodeHex, g_WildcardDB[i].barcode);
            }
        }
    }
    ImGui::EndChild();

    ImGui::SetNextItemWidth(-80);
    ImGui::InputText("##barcode", barcodeHex, sizeof(barcodeHex));
    ImGui::SameLine();
    bool canInject = !g_BarcodeInject.pending && !g_SpawnRequest.pending;
    if (!canInject) ImGui::BeginDisabled();
    if (ImGui::Button("Inject", ImVec2(-1, 0))) {
        std::string hex = barcodeHex;
        if (hex.size() >= 16) {
            // Decode and show what it is
            auto decoded = PVDecode::decode(hex);
            if (decoded.valid) {
                Log("Barcode: " + PVDecode::formatCommands(decoded), 3);
                g_BarcodeInject.hexString = hex;
                g_BarcodeInject.pending = true;
            } else {
                Log("Decode error: " + decoded.error, 5);
            }
        } else {
            Log("Enter a 16+ character hex barcode string", 5);
        }
    }
    if (!canInject) ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Paste a PV barcode hex from barcodes.txt\nExamples:\nBF619A786BD25C9B = Elite Neon Choclodocus\nF1706B7B6D69A38F = Place Choclodocus\n96FEF696AB02C4A6 = Choclodocus Egg");
    }

    // Show decoded preview if there's text in the input
    if (strlen(barcodeHex) >= 16) {
        auto preview = PVDecode::decode(std::string(barcodeHex));
        if (preview.valid) {
            ImGui::TextWrapped("Decoded: %s", PVDecode::formatCommands(preview).c_str());
        }
    }

    // Native PV pipeline inject — feeds raw barcode through game's own decoder
    if (ImGui::Button("Native PV Inject", ImVec2(-1, 0))) {
        std::string hex = barcodeHex;
        if (hex.size() >= 16) {
            // Split by space for multi-row barcodes
            size_t spacePos = hex.find(' ');
            g_NativePVInject.hexRow1 = (spacePos != std::string::npos) ? hex.substr(0, spacePos) : hex.substr(0, 16);
            g_NativePVInject.hexRow2 = (spacePos != std::string::npos && hex.size() > spacePos + 16)
                ? hex.substr(spacePos + 1, 16) : "";
            g_NativePVInject.pending = true;
            Log("NativePV: Queued row1=" + g_NativePVInject.hexRow1 +
                (g_NativePVInject.hexRow2.empty() ? "" : " row2=" + g_NativePVInject.hexRow2), 3);
        } else {
            Log("Need 16+ hex chars for native PV inject", 5);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Inject barcode through game's native PV pipeline.\nThis uses the game's own decoder — may support wildcards!");
    }

    // === Save Texture Patcher ===
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Save Texture Patcher:");

    static std::vector<SavePatcher::TextureEntry> scanResults;
    static std::string patchStatus;
    static int selectedVariant = 0;

    // Known variants for the dropdown
    static const char* dinoVariants[] = {"pink", "blue", "green", "red", "elite"};
    static const char* dragonVariants[] = {"dirt", "gold", "grass", "water", "snow", "sand"};

    if (ImGui::Button("Scan Current Save", ImVec2(-1, 0))) {
        auto savePath = SavePatcher::findSaveDir();
        if (!savePath.empty()) {
            scanResults = SavePatcher::scanSave(savePath);
            patchStatus = "Found " + std::to_string(scanResults.size()) + " variant textures in save";
        } else {
            patchStatus = "No save found!";
        }
    }

    if (!scanResults.empty()) {
        // Group by species
        std::map<std::string, std::string> speciesCurrentVariant;
        for (auto& e : scanResults) {
            speciesCurrentVariant[e.species] = e.variant;
        }

        for (auto& [species, currentVar] : speciesCurrentVariant) {
            ImGui::Text("%s: current = %s", species.c_str(), currentVar.c_str());

            const char** variants = nullptr;
            int varCount = 0;

            if (species.find("dinosaur") != std::string::npos) {
                variants = dinoVariants; varCount = 5;
            } else if (species.find("dragon") != std::string::npos) {
                variants = dragonVariants; varCount = 6;
            }

            if (variants) {
                std::string comboId = "##var_" + species;
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::Combo(comboId.c_str(), &selectedVariant, variants, varCount);
                ImGui::SameLine();
                std::string btnId = "Patch##" + species;
                if (ImGui::Button(btnId.c_str())) {
                    auto savePath = SavePatcher::findSaveDir();
                    if (!savePath.empty()) {
                        patchStatus = SavePatcher::patchSave(savePath, species, currentVar, variants[selectedVariant]);
                        // Re-scan
                        scanResults = SavePatcher::scanSave(savePath);
                    }
                }
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1,1,0,1), "(close game first!)");
            }
        }
    }

    if (!patchStatus.empty()) {
        ImGui::TextWrapped("%s", patchStatus.c_str());
    }

    ImGui::Separator();

    // Texture + File logging toggle (enables both hooks)
    extern std::ofstream g_FileLog;
    extern bool g_FileLogging;
    extern std::ofstream g_TextureLog;
    extern bool g_TextureLogging;
    if (ImGui::Button(g_TextureLogging ? "Stop Logging" : "Start Texture + File Log", ImVec2(-1, 0))) {
        g_FileLogging = !g_FileLogging;
        g_TextureLogging = !g_TextureLogging;
        if (g_TextureLogging) {
            g_FileLog.open("C:/Users/Administrator/Downloads/file_log.txt", std::ios::trunc);
            g_TextureLog.open("C:/Users/Administrator/Downloads/texture_log.txt", std::ios::trunc);
            Log("Texture + file logging started", 3);
        } else {
            g_FileLog.close();
            g_TextureLog.close();
            Log("Logging stopped — check texture_log.txt and file_log.txt", 3);
        }
    }

    // === Texture Tools ===
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Texture Tools:");

    // Combined capture + dump workflow
    if (!TextureTools::g_CaptureEnabled) {
        if (ImGui::Button("Start Texture Capture", ImVec2(-1, 0))) {
            TextureTools::g_CaptureEnabled = true;
            TextureTools::g_CapturedTextures.clear();
            g_TextureLogging = true;
            g_TextureLog.open("C:/Users/Administrator/Downloads/texture_log.txt", std::ios::trunc);
        }
    } else {
        ImGui::TextColored(ImVec4(1,1,0,1), "CAPTURING... (%d textures so far)", (int)TextureTools::g_CapturedTextures.size());
        if (ImGui::Button("Stop & Dump Textures", ImVec2(-1, 0))) {
            TextureTools::g_CaptureEnabled = false;
            g_TextureLogging = false;
            g_TextureLog.close();
            uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();
            int count = TextureTools::dumpAllTextures("C:/Users/Administrator/Downloads/texture_dump", mb);
            Log("Dumped " + std::to_string(count) + " textures", 3);
        }
    }

    // Texture swap tool: swap imageDataStart between two textures
    if (TextureTools::g_CapturedTextures.size() >= 2) {
        static int swapA = 0, swapB = 1;
        ImGui::Text("Swap textures (same format only):");
        ImGui::SetNextItemWidth(60);
        ImGui::InputInt("##swapA", &swapA);
        ImGui::SameLine();
        ImGui::Text("<->");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputInt("##swapB", &swapB);
        ImGui::SameLine();
        if (ImGui::Button("Swap!")) {
            int maxIdx = (int)TextureTools::g_CapturedTextures.size() - 1;
            if (swapA >= 0 && swapA <= maxIdx && swapB >= 0 && swapB <= maxIdx && swapA != swapB) {
                auto& a = TextureTools::g_CapturedTextures[swapA];
                auto& b = TextureTools::g_CapturedTextures[swapB];
                uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();
                // Swap imageDataStart pointers (offset +20 in dbTexture_s)
                uint32_t ptrA = std::byteswap(*(uint32_t*)(mb + a.ppcAddr + 20));
                uint32_t ptrB = std::byteswap(*(uint32_t*)(mb + b.ppcAddr + 20));
                *(uint32_t*)(mb + a.ppcAddr + 20) = std::byteswap(ptrB);
                *(uint32_t*)(mb + b.ppcAddr + 20) = std::byteswap(ptrA);

                // Also swap sizeOfOneFrame (offset +24) so dimensions match
                uint32_t sizeA = *(uint32_t*)(mb + a.ppcAddr + 24);
                uint32_t sizeB = *(uint32_t*)(mb + b.ppcAddr + 24);
                *(uint32_t*)(mb + a.ppcAddr + 24) = sizeB;
                *(uint32_t*)(mb + b.ppcAddr + 24) = sizeA;

                // Force GPU re-upload by resetting frame loaded state
                // currentFrameLoaded at +18, requiredFrame at +19
                *(mb + a.ppcAddr + 18) = 0xFF; // set to invalid
                *(mb + b.ppcAddr + 18) = 0xFF;

                // Also try invalidating the shared memory region
                // The GPU texture cache tracks which memory pages are dirty
                // Writing to the texture data should trigger a re-upload on next frame
                // Touch the first few bytes of each texture's data to mark pages dirty
                if (ptrA > 0x40000000 && ptrB > 0x40000000) {
                    uint8_t tmp = *(mb + ptrA);
                    *(mb + ptrA) = tmp; // write same value back (marks page dirty)
                    tmp = *(mb + ptrB);
                    *(mb + ptrB) = tmp;
                }

                Log("Swapped textures " + std::to_string(swapA) + " <-> " + std::to_string(swapB) + " + forced reload", 3);
            }
        }
    }

    ImGui::Separator();

    // Scan species button
    if (ImGui::Button("Scan All Species IDs (check log)", ImVec2(-1, 0))) {
        scanRequested = true;
    }

    ImGui::End();

    // Queue scan to run from the game logic hook (needs live PPC context)
    if (scanRequested) {
        scanRequested = false;
        ::g_ScanPending = true;
    }
}

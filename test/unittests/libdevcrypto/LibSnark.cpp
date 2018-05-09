/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/// @file
/// Tests for libsnark integration.

#include <libdevcrypto/LibSnark.h>
#include <libdevcore/CommonIO.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::crypto;

BOOST_AUTO_TEST_SUITE(LibSnark)

namespace dev
{
namespace test
{

namespace
{

static u256 groupOrder = u256("21888242871839275222246405745257275088548364400416034343698204186575808495617");

pair<bool, bytes> ecmul_helper(bytes const& _a, u256 const& _scalar)
{
    bytes input = _a + toBigEndian(_scalar);
    return alt_bn128_G1_mul(ref(input));
}

pair<bool, bytes> ecadd_helper(bytes const& _a, bytes const& _b)
{
    bytes input = _a + _b;
    return alt_bn128_G1_add(ref(input));
}

pair<bool, bytes> pairingprod_helper(bytes const& _input)
{
    return alt_bn128_pairing_product(ref(_input));
}

bytes negateG1(bytes const& _input)
{
    auto ret = ecmul_helper(_input, groupOrder - 1);
    BOOST_REQUIRE(ret.first);
    return ret.second;
}

bytes addG1(bytes const& _x, bytes const& _y)
{
    auto ret = ecadd_helper(_x, _y);
    BOOST_REQUIRE(ret.first);
    return ret.second;
}

}

BOOST_AUTO_TEST_CASE(ecadd)
{
    // "0 + 0 == 0"
    bytes input(0x20 * 4, 0);
    bytes expectation(0x20 * 2, 0);
    auto result = alt_bn128_G1_add(ref(input));
    BOOST_CHECK(result.first);
    BOOST_CHECK(result.second == expectation);
    // The same, truncated.
    bytes empty;
    result = alt_bn128_G1_add(ref(empty));
    BOOST_CHECK(result.first);
    BOOST_CHECK(result.second == expectation);
}

BOOST_AUTO_TEST_CASE(fieldPointInvalid)
{
    u256 const pMod{"21888242871839275222246405745257275088696311157297823662689037894645226208583"};

    bytes input = toBigEndian(pMod);
    BOOST_CHECK(!alt_bn128_G1_add(ref(input)).first);
    BOOST_CHECK(!alt_bn128_G1_mul(ref(input)).first);

    input = toBigEndian(pMod + 1);
    BOOST_CHECK(!alt_bn128_G1_add(ref(input)).first);
    BOOST_CHECK(!alt_bn128_G1_mul(ref(input)).first);

    input = bytes(32, 0) + toBigEndian(pMod);
    BOOST_CHECK(!alt_bn128_G1_add(ref(input)).first);
    BOOST_CHECK(!alt_bn128_G1_mul(ref(input)).first);

    input = bytes(32, 0) + toBigEndian(pMod + 1);
    BOOST_CHECK(!alt_bn128_G1_add(ref(input)).first);
    BOOST_CHECK(!alt_bn128_G1_mul(ref(input)).first);
}

BOOST_AUTO_TEST_CASE(invalid)
{
    bytes x =
        toBigEndian(u256("6851077925310461602867742977619883934042581405263014789956638244065803308498")) +
        toBigEndian(u256("10336382210592135525880811046708757754106524561907815205241508542912494488506"));
    bytes invalid = x;
    invalid[3] ^= 1;

    bytes input = x + invalid;
    // This should fail because the point is not on the curve
    BOOST_CHECK(!ecadd_helper(x, invalid).first);
    BOOST_CHECK(!ecadd_helper(invalid, bytes()).first);
    // truncated, but valid
    BOOST_CHECK(ecadd_helper(x, bytes()).first);
    BOOST_CHECK(ecadd_helper(x, bytes()).second == x);
}

BOOST_AUTO_TEST_CASE(ecmul_add)
{
    bytes x =
        toBigEndian(u256("6851077925310461602867742977619883934042581405263014789956638244065803308498")) +
        toBigEndian(u256("10336382210592135525880811046708757754106524561907815205241508542912494488506"));
    BOOST_CHECK(ecadd_helper(x, x).first);
    BOOST_CHECK(ecmul_helper(x, u256(2)).first);
    // x + x == x * 2
    BOOST_CHECK(ecadd_helper(x, x).second == ecmul_helper(x, u256(2)).second);
    // x * -1 + x == 0
    BOOST_CHECK(ecmul_helper(x, groupOrder - 1).first);
    BOOST_CHECK(ecadd_helper(ecmul_helper(x, groupOrder - 1).second, x).second == bytes(0x40, 0));
}

BOOST_AUTO_TEST_CASE(pairing)
{
    // This verifies a full zkSNARK proof. Let's see if this hocus-pocus actually works...
    struct VK {
        bytes A;
        bytes B;
        bytes C;
        bytes gamma;
        bytes gammaBeta1;
        bytes gammaBeta2;
        bytes Z;
        vector<bytes> IC;
    } vk;
    vk.A =
        toBigEndian(u256("0x209dd15ebff5d46c4bd888e51a93cf99a7329636c63514396b4a452003a35bf7")) +
        toBigEndian(u256("0x04bf11ca01483bfa8b34b43561848d28905960114c8ac04049af4b6315a41678")) +
        toBigEndian(u256("0x2bb8324af6cfc93537a2ad1a445cfd0ca2a71acd7ac41fadbf933c2a51be344d")) +
        toBigEndian(u256("0x120a2a4cf30c1bf9845f20c6fe39e07ea2cce61f0c9bb048165fe5e4de877550"));
    vk.B =
        toBigEndian(u256("0x2eca0c7238bf16e83e7a1e6c5d49540685ff51380f309842a98561558019fc02")) +
        toBigEndian(u256("0x03d3260361bb8451de5ff5ecd17f010ff22f5c31cdf184e9020b06fa5997db84"));
    vk.C =
        toBigEndian(u256("0x2e89718ad33c8bed92e210e81d1853435399a271913a6520736a4729cf0d51eb")) +
        toBigEndian(u256("0x01a9e2ffa2e92599b68e44de5bcf354fa2642bd4f26b259daa6f7ce3ed57aeb3")) +
        toBigEndian(u256("0x14a9a87b789a58af499b314e13c3d65bede56c07ea2d418d6874857b70763713")) +
        toBigEndian(u256("0x178fb49a2d6cd347dc58973ff49613a20757d0fcc22079f9abd10c3baee24590"));
    vk.gamma =
        toBigEndian(u256("0x25f83c8b6ab9de74e7da488ef02645c5a16a6652c3c71a15dc37fe3a5dcb7cb1")) +
        toBigEndian(u256("0x22acdedd6308e3bb230d226d16a105295f523a8a02bfc5e8bd2da135ac4c245d")) +
        toBigEndian(u256("0x065bbad92e7c4e31bf3757f1fe7362a63fbfee50e7dc68da116e67d600d9bf68")) +
        toBigEndian(u256("0x06d302580dc0661002994e7cd3a7f224e7ddc27802777486bf80f40e4ca3cfdb"));
    vk.gammaBeta1 =
        toBigEndian(u256("0x15794ab061441e51d01e94640b7e3084a07e02c78cf3103c542bc5b298669f21")) +
        toBigEndian(u256("0x14db745c6780e9df549864cec19c2daf4531f6ec0c89cc1c7436cc4d8d300c6d"));
    vk.gammaBeta2 =
        toBigEndian(u256("0x1f39e4e4afc4bc74790a4a028aff2c3d2538731fb755edefd8cb48d6ea589b5e")) +
        toBigEndian(u256("0x283f150794b6736f670d6a1033f9b46c6f5204f50813eb85c8dc4b59db1c5d39")) +
        toBigEndian(u256("0x140d97ee4d2b36d99bc49974d18ecca3e7ad51011956051b464d9e27d46cc25e")) +
        toBigEndian(u256("0x0764bb98575bd466d32db7b15f582b2d5c452b36aa394b789366e5e3ca5aabd4"));
    vk.Z =
        toBigEndian(u256("0x217cee0a9ad79a4493b5253e2e4e3a39fc2df38419f230d341f60cb064a0ac29")) +
        toBigEndian(u256("0x0a3d76f140db8418ba512272381446eb73958670f00cf46f1d9e64cba057b53c")) +
        toBigEndian(u256("0x26f64a8ec70387a13e41430ed3ee4a7db2059cc5fc13c067194bcc0cb49a9855")) +
        toBigEndian(u256("0x2fd72bd9edb657346127da132e5b82ab908f5816c826acb499e22f2412d1a2d7"));
    vk.IC.push_back(
        toBigEndian(u256("0x0aee46a7ea6e80a3675026dfa84019deee2a2dedb1bbe11d7fe124cb3efb4b5a")) +
        toBigEndian(u256("0x044747b6e9176e13ede3a4dfd0d33ccca6321b9acd23bf3683a60adc0366ebaf"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x1e39e9f0f91fa7ff8047ffd90de08785777fe61c0e3434e728fce4cf35047ddc")) +
        toBigEndian(u256("0x2e0b64d75ebfa86d7f8f8e08abbe2e7ae6e0a1c0b34d028f19fa56e9450527cb"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x1c36e713d4d54e3a9644dffca1fc524be4868f66572516025a61ca542539d43f")) +
        toBigEndian(u256("0x042dcc4525b82dfb242b09cb21909d5c22643dcdbe98c4d082cc2877e96b24db"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x17d5d09b4146424bff7e6fb01487c477bbfcd0cdbbc92d5d6457aae0b6717cc5")) +
        toBigEndian(u256("0x02b5636903efbf46db9235bbe74045d21c138897fda32e079040db1a16c1a7a1"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x0f103f14a584d4203c27c26155b2c955f8dfa816980b24ba824e1972d6486a5d")) +
        toBigEndian(u256("0x0c4165133b9f5be17c804203af781bcf168da7386620479f9b885ecbcd27b17b"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x232063b584fb76c8d07995bee3a38fa7565405f3549c6a918ddaa90ab971e7f8")) +
        toBigEndian(u256("0x2ac9b135a81d96425c92d02296322ad56ffb16299633233e4880f95aafa7fda7"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x09b54f111d3b2d1b2fe1ae9669b3db3d7bf93b70f00647e65c849275de6dc7fe")) +
        toBigEndian(u256("0x18b2e77c63a3e400d6d1f1fbc6e1a1167bbca603d34d03edea231eb0ab7b14b4"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x0c54b42137b67cc268cbb53ac62b00ecead23984092b494a88befe58445a244a")) +
        toBigEndian(u256("0x18e3723d37fae9262d58b548a0575f59d9c3266db7afb4d5739555837f6b8b3e"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x0a6de0e2240aa253f46ce0da883b61976e3588146e01c9d8976548c145fe6e4a")) +
        toBigEndian(u256("0x04fbaa3a4aed4bb77f30ebb07a3ec1c7d77a7f2edd75636babfeff97b1ea686e"))
    );
    vk.IC.push_back(
        toBigEndian(u256("0x111e2e2a5f8828f80ddad08f9f74db56dac1cc16c1cb278036f79a84cf7a116f")) +
        toBigEndian(u256("0x1d7d62e192b219b9808faa906c5ced871788f6339e8d91b83ac1343e20a16b30"))
    );
    struct Proof {
        bytes A;
        bytes Ap;
        bytes B;
        bytes Bp;
        bytes C;
        bytes Cp;
        bytes H;
        bytes K;
    } proof;
    proof.A =
        toBigEndian(u256("12873740738727497448187997291915224677121726020054032516825496230827252793177")) +
        toBigEndian(u256("21804419174137094775122804775419507726154084057848719988004616848382402162497"));
    proof.Ap =
        toBigEndian(u256("7742452358972543465462254569134860944739929848367563713587808717088650354556")) +
        toBigEndian(u256("7324522103398787664095385319014038380128814213034709026832529060148225837366"));
    proof.B =
        toBigEndian(u256("8176651290984905087450403379100573157708110416512446269839297438960217797614")) +
        toBigEndian(u256("15588556568726919713003060429893850972163943674590384915350025440408631945055")) +
        toBigEndian(u256("15347511022514187557142999444367533883366476794364262773195059233657571533367")) +
        toBigEndian(u256("4265071979090628150845437155927259896060451682253086069461962693761322642015"));
    proof.Bp =
        toBigEndian(u256("2979746655438963305714517285593753729335852012083057917022078236006592638393")) +
        toBigEndian(u256("6470627481646078059765266161088786576504622012540639992486470834383274712950"));
    proof.C =
        toBigEndian(u256("6851077925310461602867742977619883934042581405263014789956638244065803308498")) +
        toBigEndian(u256("10336382210592135525880811046708757754106524561907815205241508542912494488506"));
    proof.Cp =
        toBigEndian(u256("12491625890066296859584468664467427202390981822868257437245835716136010795448")) +
        toBigEndian(u256("13818492518017455361318553880921248537817650587494176379915981090396574171686"));
    proof.H =
        toBigEndian(u256("12091046215835229523641173286701717671667447745509192321596954139357866668225")) +
        toBigEndian(u256("14446807589950902476683545679847436767890904443411534435294953056557941441758"));
    proof.K =
        toBigEndian(u256("21341087976609916409401737322664290631992568431163400450267978471171152600502")) +
        toBigEndian(u256("2942165230690572858696920423896381470344658299915828986338281196715687693170"));
    vector<u256> input;
    input.push_back(u256("13986731495506593864492662381614386532349950841221768152838255933892789078521"));
    input.push_back(u256("622860516154313070522697309645122400675542217310916019527100517240519630053"));
    input.push_back(u256("11094488463398718754251685950409355128550342438297986977413505294941943071569"));
    input.push_back(u256("6627643779954497813586310325594578844876646808666478625705401786271515864467"));
    input.push_back(u256("2957286918163151606545409668133310005545945782087581890025685458369200827463"));
    input.push_back(u256("1384290496819542862903939282897996566903332587607290986044945365745128311081"));
    input.push_back(u256("5613571677741714971687805233468747950848449704454346829971683826953541367271"));
    input.push_back(u256("9643208548031422463313148630985736896287522941726746581856185889848792022807"));
    input.push_back(u256("18066496933330839731877828156604"));

    bytes P2 =
        toBigEndian(u256("11559732032986387107991004021392285783925812861821192530917403151452391805634")) +
        toBigEndian(u256("10857046999023057135944570762232829481370756359578518086990519993285655852781")) +
        toBigEndian(u256("4082367875863433681332203403145435568316851327593401208105741076214120093531")) +
        toBigEndian(u256("8495653923123431417604973247489272438418190587263600148770280649306958101930"));

    pair<bool, bytes> ret;
    // Compute the linear combination vk_x
    bytes vkx = toBigEndian(u256(0)) + toBigEndian(u256(0));
    for (size_t i = 0; i < input.size(); ++i)
    {
        ret = ecmul_helper(vk.IC[i + 1], input[i]);
        BOOST_REQUIRE(ret.first);
        ret = ecadd_helper(vkx, ret.second);
        BOOST_REQUIRE(ret.first);
        vkx = ret.second;
    }
    ret = ecadd_helper(vkx, vk.IC[0]);
    BOOST_REQUIRE(ret.first);
    vkx = ret.second;

    // Now run the pairing checks.
    ret = pairingprod_helper(proof.A + vk.A + negateG1(proof.Ap) + P2);
    BOOST_REQUIRE(ret.first);
    BOOST_REQUIRE(ret.second == toBigEndian(u256(1)));
    ret = pairingprod_helper(vk.B + proof.B + negateG1(proof.Bp) + P2);
    BOOST_REQUIRE(ret.first);
    BOOST_REQUIRE(ret.second == toBigEndian(u256(1)));
    ret = pairingprod_helper(proof.C + vk.C + negateG1(proof.Cp) + P2);
    BOOST_REQUIRE(ret.first);
    BOOST_REQUIRE(ret.second == toBigEndian(u256(1)));
    ret = pairingprod_helper(
        proof.K + vk.gamma +
        negateG1(addG1(vkx, addG1(proof.A, proof.C))) + vk.gammaBeta2 +
        negateG1(vk.gammaBeta1) + proof.B
    );
    BOOST_REQUIRE(ret.first);
    BOOST_REQUIRE(ret.second == toBigEndian(u256(1)));
    ret = pairingprod_helper(
        addG1(vkx, proof.A) + proof.B +
        negateG1(proof.H) + vk.Z +
        negateG1(proof.C) + P2
    );
    BOOST_REQUIRE(ret.first);
    BOOST_REQUIRE(ret.second == toBigEndian(u256(1)));

    // Just for the fun of it, try a wrong check.
    ret = pairingprod_helper(proof.A + vk.A + proof.Ap + P2);
    BOOST_REQUIRE(ret.first);
    BOOST_REQUIRE(ret.second == toBigEndian(u256(0)));
}

BOOST_AUTO_TEST_CASE(pairingNullInput)
{
    // TODO: Maybe the empty input should also be considered invalid?
    auto r = pairingprod_helper({});
    BOOST_CHECK(r.first);

    r = pairingprod_helper(bytes(2 * 32 + 2 * 64, 0));
    BOOST_CHECK(r.first);

    // Invalid length of input.
    r = pairingprod_helper(bytes(2 * 32 + 2 * 64 + 1, 0));
    BOOST_CHECK(!r.first);
}

BOOST_AUTO_TEST_CASE(generateRandomPoints)
{
    bytes trivialPt = toBigEndian(u256(1)) + toBigEndian(u256(2));

    bool success = false;
    bytes output;
    bytes input = trivialPt + toBigEndian(u256(1));
    std::tie(success, output) = alt_bn128_G1_mul(ref(input));
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(toHex(output), toHex(trivialPt));

    input = trivialPt + toBigEndian(u256(123454352435654643));
    std::tie(success, output) = alt_bn128_G1_mul(ref(input));
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(toHex(output), "18b18acfb4c2c30276db5411368e7185b311dd124691610c5d3b74034e093dc9063c909c4720840cb5134cb9f59fa749755796819658d32efc0d288198f37266");
    bytes a = std::move(output);

    input = trivialPt + toBigEndian(u256(653179013456575642));
    std::tie(success, output) = alt_bn128_G1_mul(ref(input));
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(toHex(output), "07c2b7f58a84bd6145f00c9c2bc0bb1a187f20ff2c92963a88019e7c6a014eed06614e20c147e940f2d70da3f74c9a17df361706a4485c742bd6788478fa17d7");
    bytes b = std::move(output);

    input = a + b;
    BOOST_CHECK_EQUAL(toHex(input), "18b18acfb4c2c30276db5411368e7185b311dd124691610c5d3b74034e093dc9063c909c4720840cb5134cb9f59fa749755796819658d32efc0d288198f3726607c2b7f58a84bd6145f00c9c2bc0bb1a187f20ff2c92963a88019e7c6a014eed06614e20c147e940f2d70da3f74c9a17df361706a4485c742bd6788478fa17d7");
    std::tie(success, output) = alt_bn128_G1_add(ref(input));
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(toHex(output), "2243525c5efd4b9c3d3c45ac0ca3fe4dd85e830a4ce6b65fa1eeaee202839703301d1d33be6da8e509df21cc35964723180eed7532537db9ae5e7d48f195c915");
    bytes c = std::move(output);

    input = c + a;
    BOOST_CHECK_EQUAL(toHex(input), "2243525c5efd4b9c3d3c45ac0ca3fe4dd85e830a4ce6b65fa1eeaee202839703301d1d33be6da8e509df21cc35964723180eed7532537db9ae5e7d48f195c91518b18acfb4c2c30276db5411368e7185b311dd124691610c5d3b74034e093dc9063c909c4720840cb5134cb9f59fa749755796819658d32efc0d288198f37266");
    std::tie(success, output) = alt_bn128_G1_add(ref(input));
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(toHex(output), "2bd3e6d0f3b142924f5ca7b49ce5b9d54c4703d7ae5648e61d02268b1a0a9fb721611ce0a6af85915e2f1d70300909ce2e49dfad4a4619c8390cae66cefdb204");
    bytes d = std::move(output);

    input = d + toBigEndian(u256(1230482048326178242));
    BOOST_CHECK_EQUAL(toHex(input), "2bd3e6d0f3b142924f5ca7b49ce5b9d54c4703d7ae5648e61d02268b1a0a9fb721611ce0a6af85915e2f1d70300909ce2e49dfad4a4619c8390cae66cefdb20400000000000000000000000000000000000000000000000011138ce750fa15c2");
    std::tie(success, output) = alt_bn128_G1_mul(ref(input));
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(toHex(output), "070a8d6a982153cae4be29d434e8faef8a47b274a053f5a4ee2a6c9c13c31e5c031b8ce914eba3a9ffb989f9cdd5b0f01943074bf4f0f315690ec3cec6981afc");
    bytes e = std::move(output);

    // Multiply by (p - 1).
    input = e + toBigEndian(u256("21888242871839275222246405745257275088696311157297823662689037894645226208582"));
    BOOST_CHECK_EQUAL(toHex(input), "070a8d6a982153cae4be29d434e8faef8a47b274a053f5a4ee2a6c9c13c31e5c031b8ce914eba3a9ffb989f9cdd5b0f01943074bf4f0f315690ec3cec6981afc30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd46");
    std::tie(success, output) = alt_bn128_G1_mul(ref(input));
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(toHex(output), "025a6f4181d2b4ea8b724290ffb40156eb0adb514c688556eb79cdea0752c2bb2eff3f31dea215f1eb86023a133a996eb6300b44da664d64251d05381bb8a02e");
    bytes f = std::move(output);

    // Multiply by (p - 1) / 2.
    input = f + toBigEndian(u256("10944121435919637611123202872628637544348155578648911831344518947322613104291"));
    BOOST_CHECK_EQUAL(toHex(input), "025a6f4181d2b4ea8b724290ffb40156eb0adb514c688556eb79cdea0752c2bb2eff3f31dea215f1eb86023a133a996eb6300b44da664d64251d05381bb8a02e183227397098d014dc2822db40c0ac2ecbc0b548b438e5469e10460b6c3e7ea3");
    std::tie(success, output) = alt_bn128_G1_mul(ref(input));
    BOOST_REQUIRE(success);
    BOOST_CHECK_EQUAL(toHex(output), "14789d0d4a730b354403b5fac948113739e276c23e0258d8596ee72f9cd9d3230af18a63153e0ec25ff9f2951dd3fa90ed0197bfef6e2a1a62b5095b9d2b4a27");
}

BOOST_AUTO_TEST_CASE(benchECADD)
{
    bytes v = fromHex("18b18acfb4c2c30276db5411368e7185b311dd124691610c5d3b74034e093dc9063c909c4720840cb5134cb9f59fa749755796819658d32efc0d288198f37266");
    bytes w = fromHex("07c2b7f58a84bd6145f00c9c2bc0bb1a187f20ff2c92963a88019e7c6a014eed06614e20c147e940f2d70da3f74c9a17df361706a4485c742bd6788478fa17d7");

    for (int i = 0; i < 10000; ++i)
    {
        bool success = false;
        bytes output;
        std::tie(success, output) = ecadd_helper(v, w);
        BOOST_REQUIRE(success);
        w = std::move(v);
        v = std::move(output);
    }

    BOOST_CHECK_EQUAL(toHex(v), "07ac1ef4e12f6bc2a1c780aa31e3cbea913a3e24d92f0a936817fe825319ae2a2541c130d13107fd99e2aca8bb647f0a098449c9a45ef44be0c9dc5ad298f355");
    BOOST_CHECK_EQUAL(toHex(w), "002d85fbe013fa580e85705ec058f4868db26e740e5262ac4bb7e2a579d61f381d56f11fb11cc3f6df6fe8f7dcd8a6483424ec9496fcfecf2fbb2c0c32955572");
}

BOOST_AUTO_TEST_CASE(benchECMULRand)
{
    bytes v = fromHex("1fa111cf23c269b75957c715b762ef37074d341c280d113707ff342211b794571db10707e7cb4ba3c851f6bbb43399701da0c7675ca0f9cfc595774fd055b2fb");
    u256 k = 1;

    for (int i = 0; i < 1000; ++i)
    {
        k = k * 6364136223846793005 + 1442695040888963407;
        bool success = false;
        bytes output;
        std::tie(success, output) = ecmul_helper(v, k);
        BOOST_REQUIRE(success);
        v = std::move(output);
    }

    BOOST_CHECK_EQUAL(toHex(v), "0e9138e9515b63654de63453e2473362f9e1ef3b457f1b671dcb0513fc43c3b51dd4b43630c4b2835266c0c5c315d546efb53da04dc89fc6f125ff9958b0c7d2");
}

BOOST_AUTO_TEST_CASE(benchECMULWorstCase1)
{
    bytes v = fromHex("1fa111cf23c269b75957c715b762ef37074d341c280d113707ff342211b794571db10707e7cb4ba3c851f6bbb43399701da0c7675ca0f9cfc595774fd055b2fb");
    u256 const k{"21888242871839275222246405745257275088696311157297823662689037894645226208582"};

    for (int i = 0; i < 1000; ++i)
    {
        bool success = false;
        bytes output;
        std::tie(success, output) = ecmul_helper(v, k);
        BOOST_REQUIRE(success);
        v = std::move(output);
    }

    BOOST_CHECK_EQUAL(toHex(v), "1701eb8c738f0f3a531d9b5468cbb7a9bb298c93d7c462bc5b06c69e78ee054927e136cbd59c7c29f6333105cee10066e0e1f83ecf376d97059cc311f82bdbd1");
}

BOOST_AUTO_TEST_CASE(benchECMULWorstCase2)
{
    bytes v = fromHex("1fa111cf23c269b75957c715b762ef37074d341c280d113707ff342211b794571db10707e7cb4ba3c851f6bbb43399701da0c7675ca0f9cfc595774fd055b2fb");
    u256 const k{"10944121435919637611123202872628637544348155578648911831344518947322613104291"};

    for (int i = 0; i < 1000; ++i)
    {
        bool success = false;
        bytes output;
        std::tie(success, output) = ecmul_helper(v, k);
        BOOST_REQUIRE(success);
        v = std::move(output);
    }

    BOOST_CHECK_EQUAL(toHex(v), "002dd4014cc80a8b493f703a8cb6940b54795923e146ef15d264a7627c790d56303c653f248c7612e90683508da0f4a24dcc26825d1a124c0b912a9a217e0b0e");
}

BOOST_AUTO_TEST_CASE(benchECMULIdentity)
{
    bytes const w = fromHex("1fa111cf23c269b75957c715b762ef37074d341c280d113707ff342211b794571db10707e7cb4ba3c851f6bbb43399701da0c7675ca0f9cfc595774fd055b2fb");
    u256 const k = 1;
    bytes v = w;

    for (int i = 0; i < 1000; ++i)
    {
        bool success = false;
        bytes output;
        std::tie(success, output) = ecmul_helper(v, k);
        BOOST_REQUIRE(success);
        v = std::move(output);
    }

    BOOST_CHECK_EQUAL(toHex(v), toHex(w));
}

BOOST_AUTO_TEST_CASE(ECMULuseCaseFromRopsten)
{
    bytes const input = fromHex("277a420332215ead37ba61fee84f0d216a345e762af8efd15453697170b3cdc5"
                                "1b312cd37d4ad474fc299c9689fc0f347a2ec2b5b474a41b343142ee5fdd097a"
                                "000000000000000000000000000000000000000000000000002386f26fc10000");
    bool success = false;
    bytes output;
    std::tie(success, output) = alt_bn128_G1_mul(ref(input));
    BOOST_REQUIRE(success);
    BOOST_REQUIRE_EQUAL(toHex(output), "0868bea7e99c34c3a289a668bc94374858954890f16469dae702bb55ed06cff2"
                                       "28aa1caa7a495965f8298537ca6221728910b41315514ff4db928688f7d40546");
}

BOOST_AUTO_TEST_SUITE_END()

}
}

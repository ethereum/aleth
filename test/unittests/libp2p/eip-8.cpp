// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Forward-compatibility tests checking EIP-8 compliance.

#include <libp2p/Host.h>
#include <libp2p/NodeTable.h>
#include <libp2p/RLPXSocket.h>
#include <libp2p/RLPxHandshake.h>
#include <gtest/gtest.h>

using namespace std;
using namespace dev;
using namespace dev::p2p;

TEST(eip8, test_discovery_packets)
{
    bi::udp::endpoint ep;
    bytes packet;

    packet = fromHex(
        "e9614ccfd9fc3e74360018522d30e1419a143407ffcce748de3e22116b7e8dc92ff74788c0b6663a"
        "aa3d67d641936511c8f8d6ad8698b820a7cf9e1be7155e9a241f556658c55428ec0563514365799a"
        "4be2be5a685a80971ddcfa80cb422cdd0101ec04cb847f000001820cfa8215a8d790000000000000"
        "000000000000000000018208ae820d058443b9a3550102");
    auto ping1 =
        dynamic_cast<PingNode&>(*DiscoveryDatagram::interpretUDP(ep, bytesConstRef(&packet)));
    EXPECT_EQ(ping1.version, 4);
    EXPECT_EQ(*ping1.expiration, 1136239445);
    EXPECT_EQ(ping1.source, NodeIPEndpoint(bi::address::from_string("127.0.0.1"), 3322, 5544));
    EXPECT_EQ(ping1.destination, NodeIPEndpoint(bi::address::from_string("::1"), 2222, 3333));

    packet = fromHex(
        "577be4349c4dd26768081f58de4c6f375a7a22f3f7adda654d1428637412c3d7fe917cadc56d4e5e"
        "7ffae1dbe3efffb9849feb71b262de37977e7c7a44e677295680e9e38ab26bee2fcbae207fba3ff3"
        "d74069a50b902a82c9903ed37cc993c50001f83e82022bd79020010db83c4d001500000000abcdef"
        "12820cfa8215a8d79020010db885a308d313198a2e037073488208ae82823a8443b9a355c5010203"
        "040531b9019afde696e582a78fa8d95ea13ce3297d4afb8ba6433e4154caa5ac6431af1b80ba7602"
        "3fa4090c408f6b4bc3701562c031041d4702971d102c9ab7fa5eed4cd6bab8f7af956f7d565ee191"
        "7084a95398b6a21eac920fe3dd1345ec0a7ef39367ee69ddf092cbfe5b93e5e568ebc491983c09c7"
        "6d922dc3");
    auto ping2 =
        dynamic_cast<PingNode&>(*DiscoveryDatagram::interpretUDP(ep, bytesConstRef(&packet)));
    EXPECT_EQ(ping2.version, 555);
    EXPECT_EQ(ping2.source,
        NodeIPEndpoint(bi::address::from_string("2001:db8:3c4d:15::abcd:ef12"), 3322, 5544));
    EXPECT_EQ(ping2.destination,
        NodeIPEndpoint(
            bi::address::from_string("2001:db8:85a3:8d3:1319:8a2e:370:7348"), 2222, 33338));
    EXPECT_EQ(*ping2.expiration, 1136239445);

    packet = fromHex(
        "09b2428d83348d27cdf7064ad9024f526cebc19e4958f0fdad87c15eb598dd61d08423e0bf66b206"
        "9869e1724125f820d851c136684082774f870e614d95a2855d000f05d1648b2d5945470bc187c2d2"
        "216fbe870f43ed0909009882e176a46b0102f846d79020010db885a308d313198a2e037073488208"
        "ae82823aa0fbc914b16819237dcd8801d7e53f69e9719adecb3cc0e790c57e91ca4461c9548443b9"
        "a355c6010203c2040506a0c969a58f6f9095004c0177a6b47f451530cab38966a25cca5cb58f0555"
        "42124e");
    auto pong = dynamic_cast<Pong&>(*DiscoveryDatagram::interpretUDP(ep, bytesConstRef(&packet)));
    EXPECT_EQ(pong.destination,
        NodeIPEndpoint(
            bi::address::from_string("2001:db8:85a3:8d3:1319:8a2e:370:7348"), 2222, 33338));
    EXPECT_EQ(pong.echo, h256("fbc914b16819237dcd8801d7e53f69e9719adecb3cc0e790c57e91ca4461c954"));
    EXPECT_EQ(*pong.expiration, 1136239445);

    packet = fromHex(
        "c7c44041b9f7c7e41934417ebac9a8e1a4c6298f74553f2fcfdcae6ed6fe53163eb3d2b52e39fe91"
        "831b8a927bf4fc222c3902202027e5e9eb812195f95d20061ef5cd31d502e47ecb61183f74a504fe"
        "04c51e73df81f25c4d506b26db4517490103f84eb840ca634cae0d49acb401d8a4c6b6fe8c55b70d"
        "115bf400769cc1400f3258cd31387574077f301b421bc84df7266c44e9e6d569fc56be0081290476"
        "7bf5ccd1fc7f8443b9a35582999983999999280dc62cc8255c73471e0a61da0c89acdc0e035e260a"
        "dd7fc0c04ad9ebf3919644c91cb247affc82b69bd2ca235c71eab8e49737c937a2c396");
    auto findnode =
        dynamic_cast<FindNode&>(*DiscoveryDatagram::interpretUDP(ep, bytesConstRef(&packet)));
    EXPECT_EQ(
        findnode.target, Public("ca634cae0d49acb401d8a4c6b6fe8c55b70d115bf400769cc1400f3258cd313875"
                                "74077f301b421bc84df7266c44e9e6d569fc56be00812904767bf5ccd1fc7f"));
    EXPECT_EQ(*findnode.expiration, 1136239445);

    packet = fromHex(
        "c679fc8fe0b8b12f06577f2e802d34f6fa257e6137a995f6f4cbfc9ee50ed3710faf6e66f932c4c8"
        "d81d64343f429651328758b47d3dbc02c4042f0fff6946a50f4a49037a72bb550f3a7872363a83e1"
        "b9ee6469856c24eb4ef80b7535bcf99c0004f9015bf90150f84d846321163782115c82115db84031"
        "55e1427f85f10a5c9a7755877748041af1bcd8d474ec065eb33df57a97babf54bfd2103575fa8291"
        "15d224c523596b401065a97f74010610fce76382c0bf32f84984010203040101b840312c55512422"
        "cf9b8a4097e9a6ad79402e87a15ae909a4bfefa22398f03d20951933beea1e4dfa6f968212385e82"
        "9f04c2d314fc2d4e255e0d3bc08792b069dbf8599020010db83c4d001500000000abcdef12820d05"
        "820d05b84038643200b172dcfef857492156971f0e6aa2c538d8b74010f8e140811d53b98c765dd2"
        "d96126051913f44582e8c199ad7c6d6819e9a56483f637feaac9448aacf8599020010db885a308d3"
        "13198a2e037073488203e78203e8b8408dcab8618c3253b558d459da53bd8fa68935a719aff8b811"
        "197101a4b2b47dd2d47295286fc00cc081bb542d760717d1bdd6bec2c37cd72eca367d6dd3b9df73"
        "8443b9a355010203b525a138aa34383fec3d2719a0");
    auto neighbours =
        dynamic_cast<Neighbours&>(*DiscoveryDatagram::interpretUDP(ep, bytesConstRef(&packet)));
    EXPECT_EQ(neighbours.neighbours.size(), 4);
    EXPECT_EQ(neighbours.neighbours[0].endpoint,
        NodeIPEndpoint(bi::address::from_string("99.33.22.55"), 4444, 4445));
    EXPECT_EQ(neighbours.neighbours[0].node,
        Public("3155e1427f85f10a5c9a7755877748041af1bcd8d474ec065eb33df57a97babf54bfd2103575fa82911"
               "5d224c523596b401065a97f74010610fce76382c0bf32"));
    EXPECT_EQ(neighbours.neighbours[1].endpoint,
        NodeIPEndpoint(bi::address::from_string("1.2.3.4"), 1, 1));
    EXPECT_EQ(neighbours.neighbours[1].node,
        Public("312c55512422cf9b8a4097e9a6ad79402e87a15ae909a4bfefa22398f03d20951933beea1e4dfa6f968"
               "212385e829f04c2d314fc2d4e255e0d3bc08792b069db"));
    EXPECT_EQ(neighbours.neighbours[2].endpoint,
        NodeIPEndpoint(bi::address::from_string("2001:db8:3c4d:15::abcd:ef12"), 3333, 3333));
    EXPECT_EQ(neighbours.neighbours[2].node,
        Public("38643200b172dcfef857492156971f0e6aa2c538d8b74010f8e140811d53b98c765dd2d96126051913f"
               "44582e8c199ad7c6d6819e9a56483f637feaac9448aac"));
    EXPECT_EQ(neighbours.neighbours[3].endpoint,
        NodeIPEndpoint(
            bi::address::from_string("2001:db8:85a3:8d3:1319:8a2e:370:7348"), 999, 1000));
    EXPECT_EQ(neighbours.neighbours[3].node,
        Public("8dcab8618c3253b558d459da53bd8fa68935a719aff8b811197101a4b2b47dd2d47295286fc00cc081b"
               "b542d760717d1bdd6bec2c37cd72eca367d6dd3b9df73"));
    EXPECT_EQ(*neighbours.expiration, 1136239445);
}

class TestHandshake : public RLPXHandshake
{
public:
    TestHandshake(Host* _host, std::shared_ptr<RLPXSocket> const& _socket)
      : RLPXHandshake(_host, _socket){};
    TestHandshake(Host* _host, std::shared_ptr<RLPXSocket> const& _socket, NodeID _remote)
      : RLPXHandshake(_host, _socket, _remote){};

    /// Creates a handshake attached to a Host with the given alias,
    /// then runs it through the key establishment, supplying packet
    /// as the input on the socket.
    ///
    /// If remoteID is supplied, the handshake runs in initiator mode.
    static shared_ptr<TestHandshake> runWithInput(
        Secret _hostAlias, bytes _packet, NodeID _remoteID = NodeID());

    /// transition is overridden to stop after key establishment.
    virtual void transition(boost::system::error_code _ec);

    /// Reports whether we made it through the key establishment without error.
    bool completedKeyEstablishment() { return m_nextState == ReadHello; }

    /// Checks whether Auth-related members match the values in the EIP-8 test vectors.
    void checkAuthValuesEIP8(uint64_t _expectedRemoteVersion);

    /// Checks whether Ack-related members match the values in the EIP-8 test vectors.
    void checkAckValuesEIP8(uint64_t _expectedRemoteVersion);
};

// This test checks that pre-EIP-8 'plain' format is still accepted.
//
// TODO - This test appears to be consistently broken on Windows, so we will
// disable it for the time being, so that we have "clean" tests to iterate on.
// With any tests in a broken state, the automation is not useful, because
// we do not see the working -> broken transition occur.
//
// See https://github.com/ethereum/cpp-ethereum/issues/3273

#if !defined(_WIN32)
TEST(eip8, test_handshake_plain_auth)
{
    Secret keyB("b71c71a67e1177ad4e901695e1b4b9ee17ae16c6668d313eac2f96dbcda3f291");
    bytes auth(
        fromHex("048ca79ad18e4b0659fab4853fe5bc58eb83992980f4c9cc147d2aa31532efd29a3d3dc6a3d89eaf"
                "913150cfc777ce0ce4af2758bf4810235f6e6ceccfee1acc6b22c005e9e3a49d6448610a58e98744"
                "ba3ac0399e82692d67c1f58849050b3024e21a52c9d3b01d871ff5f210817912773e610443a9ef14"
                "2e91cdba0bd77b5fdf0769b05671fc35f83d83e4d3b0b000c6b2a1b1bba89e0fc51bf4e460df3105"
                "c444f14be226458940d6061c296350937ffd5e3acaceeaaefd3c6f74be8e23e0f45163cc7ebd7622"
                "0f0128410fd05250273156d548a414444ae2f7dea4dfca2d43c057adb701a715bf59f6fb66b2d1d2"
                "0f2c703f851cbf5ac47396d9ca65b6260bd141ac4d53e2de585a73d1750780db4c9ee4cd4d225173"
                "a4592ee77e2bd94d0be3691f3b406f9bba9b591fc63facc016bfa8"));
    shared_ptr<TestHandshake> h = TestHandshake::runWithInput(keyB, auth);
    EXPECT_TRUE(h->completedKeyEstablishment());
    h->checkAuthValuesEIP8(4);
}
#endif  // !defined(_WIN32)

// TODO - This test appears to be consistently broken on Windows, so we will
// disable it for the time being, so that we have "clean" tests to iterate on.
// With any tests in a broken state, the automation is not useful, because
// we do not see the working -> broken transition occur.
//
// See https://github.com/ethereum/cpp-ethereum/issues/3273

#if !defined(_WIN32)
TEST(eip8, test_handshake_eip8_auth1)
{
    Secret keyB("b71c71a67e1177ad4e901695e1b4b9ee17ae16c6668d313eac2f96dbcda3f291");
    bytes auth(
        fromHex("01b304ab7578555167be8154d5cc456f567d5ba302662433674222360f08d5f1534499d3678b513b"
                "0fca474f3a514b18e75683032eb63fccb16c156dc6eb2c0b1593f0d84ac74f6e475f1b8d56116b84"
                "9634a8c458705bf83a626ea0384d4d7341aae591fae42ce6bd5c850bfe0b999a694a49bbbaf3ef6c"
                "da61110601d3b4c02ab6c30437257a6e0117792631a4b47c1d52fc0f8f89caadeb7d02770bf999cc"
                "147d2df3b62e1ffb2c9d8c125a3984865356266bca11ce7d3a688663a51d82defaa8aad69da39ab6"
                "d5470e81ec5f2a7a47fb865ff7cca21516f9299a07b1bc63ba56c7a1a892112841ca44b6e0034dee"
                "70c9adabc15d76a54f443593fafdc3b27af8059703f88928e199cb122362a4b35f62386da7caad09"
                "c001edaeb5f8a06d2b26fb6cb93c52a9fca51853b68193916982358fe1e5369e249875bb8d0d0ec3"
                "6f917bc5e1eafd5896d46bd61ff23f1a863a8a8dcd54c7b109b771c8e61ec9c8908c733c0263440e"
                "2aa067241aaa433f0bb053c7b31a838504b148f570c0ad62837129e547678c5190341e4f1693956c"
                "3bf7678318e2d5b5340c9e488eefea198576344afbdf66db5f51204a6961a63ce072c8926c"));
    shared_ptr<TestHandshake> h = TestHandshake::runWithInput(keyB, auth);
    EXPECT_TRUE(h->completedKeyEstablishment());
    h->checkAuthValuesEIP8(4);
}
#endif  // !defined(_WIN32)

// TODO - This test appears to be consistently broken on Windows, so we will
// disable it for the time being, so that we have "clean" tests to iterate on.
// With any tests in a broken state, the automation is not useful, because
// we do not see the working -> broken transition occur.
//
// See https://github.com/ethereum/cpp-ethereum/issues/3273

#if !defined(_WIN32)
TEST(eip8, test_handshake_eip8_auth2)
{
    Secret keyB("b71c71a67e1177ad4e901695e1b4b9ee17ae16c6668d313eac2f96dbcda3f291");
    bytes auth(
        fromHex("01b8044c6c312173685d1edd268aa95e1d495474c6959bcdd10067ba4c9013df9e40ff45f5bfd6f7"
                "2471f93a91b493f8e00abc4b80f682973de715d77ba3a005a242eb859f9a211d93a347fa64b597bf"
                "280a6b88e26299cf263b01b8dfdb712278464fd1c25840b995e84d367d743f66c0e54a586725b7bb"
                "f12acca27170ae3283c1073adda4b6d79f27656993aefccf16e0d0409fe07db2dc398a1b7e8ee93b"
                "cd181485fd332f381d6a050fba4c7641a5112ac1b0b61168d20f01b479e19adf7fdbfa0905f63352"
                "bfc7e23cf3357657455119d879c78d3cf8c8c06375f3f7d4861aa02a122467e069acaf513025ff19"
                "6641f6d2810ce493f51bee9c966b15c5043505350392b57645385a18c78f14669cc4d960446c1757"
                "1b7c5d725021babbcd786957f3d17089c084907bda22c2b2675b4378b114c601d858802a55345a15"
                "116bc61da4193996187ed70d16730e9ae6b3bb8787ebcaea1871d850997ddc08b4f4ea668fbf3740"
                "7ac044b55be0908ecb94d4ed172ece66fd31bfdadf2b97a8bc690163ee11f5b575a4b44e36e2bfb2"
                "f0fce91676fd64c7773bac6a003f481fddd0bae0a1f31aa27504e2a533af4cef3b623f4791b2cca6"
                "d490"));
    shared_ptr<TestHandshake> h = TestHandshake::runWithInput(keyB, auth);
    EXPECT_TRUE(h->completedKeyEstablishment());
    h->checkAuthValuesEIP8(56);
}
#endif  // !defined(_WIN32)

// TODO - This test appears to be consistently broken on Windows, so we will
// disable it for the time being, so that we have "clean" tests to iterate on.
// With any tests in a broken state, the automation is not useful, because
// we do not see the working -> broken transition occur.
//
// See https://github.com/ethereum/cpp-ethereum/issues/3273

#if !defined(_WIN32)
TEST(eip8, test_handshake_eip8_ack_plain)
{
    Secret keyA("49a7b37aa6f6645917e7b807e9d1c00d4fa71f18343b0d4122a4d2df64dd6fee");
    bytes ack(
        fromHex("049f8abcfa9c0dc65b982e98af921bc0ba6e4243169348a236abe9df5f93aa69d99cadddaa387662"
                "b0ff2c08e9006d5a11a278b1b3331e5aaabf0a32f01281b6f4ede0e09a2d5f585b26513cb794d963"
                "5a57563921c04a9090b4f14ee42be1a5461049af4ea7a7f49bf4c97a352d39c8d02ee4acc416388c"
                "1c66cec761d2bc1c72da6ba143477f049c9d2dde846c252c111b904f630ac98e51609b3b1f58168d"
                "dca6505b7196532e5f85b259a20c45e1979491683fee108e9660edbf38f3add489ae73e3dda2c71b"
                "d1497113d5c755e942d1"));
    NodeID initiatorPubk(
        "fda1cff674c90c9a197539fe3dfb53086ace64f83ed7c6eabec741f7f381cc803e52ab2cd55d5569bce4347107"
        "a310dfd5f88a010cd2ffd1005ca406f1842877");
    shared_ptr<TestHandshake> h = TestHandshake::runWithInput(keyA, ack, initiatorPubk);
    EXPECT_TRUE(h->completedKeyEstablishment());
    h->checkAckValuesEIP8(4);
}
#endif  // !defined(_WIN32)

// TODO - This test appears to be consistently broken on Windows, so we will
// disable it for the time being, so that we have "clean" tests to iterate on.
// With any tests in a broken state, the automation is not useful, because
// we do not see the working -> broken transition occur.
//
// See https://github.com/ethereum/cpp-ethereum/issues/3273

#if !defined(_WIN32)
TEST(eip8, test_handshake_eip8_ack1)
{
    Secret keyA("49a7b37aa6f6645917e7b807e9d1c00d4fa71f18343b0d4122a4d2df64dd6fee");
    bytes ack(
        fromHex("01ea0451958701280a56482929d3b0757da8f7fbe5286784beead59d95089c217c9b917788989470"
                "b0e330cc6e4fb383c0340ed85fab836ec9fb8a49672712aeabbdfd1e837c1ff4cace34311cd7f4de"
                "05d59279e3524ab26ef753a0095637ac88f2b499b9914b5f64e143eae548a1066e14cd2f4bd7f814"
                "c4652f11b254f8a2d0191e2f5546fae6055694aed14d906df79ad3b407d94692694e259191cde171"
                "ad542fc588fa2b7333313d82a9f887332f1dfc36cea03f831cb9a23fea05b33deb999e85489e645f"
                "6aab1872475d488d7bd6c7c120caf28dbfc5d6833888155ed69d34dbdc39c1f299be1057810f34fb"
                "e754d021bfca14dc989753d61c413d261934e1a9c67ee060a25eefb54e81a4d14baff922180c395d"
                "3f998d70f46f6b58306f969627ae364497e73fc27f6d17ae45a413d322cb8814276be6ddd13b885b"
                "201b943213656cde498fa0e9ddc8e0b8f8a53824fbd82254f3e2c17e8eaea009c38b4aa0a3f306e8"
                "797db43c25d68e86f262e564086f59a2fc60511c42abfb3057c247a8a8fe4fb3ccbadde17514b7ac"
                "8000cdb6a912778426260c47f38919a91f25f4b5ffb455d6aaaf150f7e5529c100ce62d6d92826a7"
                "1778d809bdf60232ae21ce8a437eca8223f45ac37f6487452ce626f549b3b5fdee26afd2072e4bc7"
                "5833c2464c805246155289f4"));
    NodeID initiatorPubk(
        "fda1cff674c90c9a197539fe3dfb53086ace64f83ed7c6eabec741f7f381cc803e52ab2cd55d5569bce4347107"
        "a310dfd5f88a010cd2ffd1005ca406f1842877");
    shared_ptr<TestHandshake> h = TestHandshake::runWithInput(keyA, ack, initiatorPubk);
    EXPECT_TRUE(h->completedKeyEstablishment());
    h->checkAckValuesEIP8(4);
}
#endif  // !defined(_WIN32)

// TODO - This test appears to be consistently broken on Windows, so we will
// disable it for the time being, so that we have "clean" tests to iterate on.
// With any tests in a broken state, the automation is not useful, because
// we do not see the working -> broken transition occur.
//
// See https://github.com/ethereum/cpp-ethereum/issues/3273

#if !defined(_WIN32)
TEST(eip8, test_handshake_eip8_ack2)
{
    Secret keyA("49a7b37aa6f6645917e7b807e9d1c00d4fa71f18343b0d4122a4d2df64dd6fee");
    bytes ack(
        fromHex("01f004076e58aae772bb101ab1a8e64e01ee96e64857ce82b1113817c6cdd52c09d26f7b90981cd7"
                "ae835aeac72e1573b8a0225dd56d157a010846d888dac7464baf53f2ad4e3d584531fa203658fab0"
                "3a06c9fd5e35737e417bc28c1cbf5e5dfc666de7090f69c3b29754725f84f75382891c561040ea1d"
                "dc0d8f381ed1b9d0d4ad2a0ec021421d847820d6fa0ba66eaf58175f1b235e851c7e2124069fbc20"
                "2888ddb3ac4d56bcbd1b9b7eab59e78f2e2d400905050f4a92dec1c4bdf797b3fc9b2f8e84a482f3"
                "d800386186712dae00d5c386ec9387a5e9c9a1aca5a573ca91082c7d68421f388e79127a5177d4f8"
                "590237364fd348c9611fa39f78dcdceee3f390f07991b7b47e1daa3ebcb6ccc9607811cb17ce51f1"
                "c8c2c5098dbdd28fca547b3f58c01a424ac05f869f49c6a34672ea2cbbc558428aa1fe48bbfd6115"
                "8b1b735a65d99f21e70dbc020bfdface9f724a0d1fb5895db971cc81aa7608baa0920abb0a565c9c"
                "436e2fd13323428296c86385f2384e408a31e104670df0791d93e743a3a5194ee6b076fb6323ca59"
                "3011b7348c16cf58f66b9633906ba54a2ee803187344b394f75dd2e663a57b956cb830dd7a908d4f"
                "39a2336a61ef9fda549180d4ccde21514d117b6c6fd07a9102b5efe710a32af4eeacae2cb3b1dec0"
                "35b9593b48b9d3ca4c13d245d5f04169b0b1"));
    NodeID initiatorPubk(
        "fda1cff674c90c9a197539fe3dfb53086ace64f83ed7c6eabec741f7f381cc803e52ab2cd55d5569bce4347107"
        "a310dfd5f88a010cd2ffd1005ca406f1842877");
    shared_ptr<TestHandshake> h = TestHandshake::runWithInput(keyA, ack, initiatorPubk);
    EXPECT_TRUE(h->completedKeyEstablishment());
    h->checkAckValuesEIP8(57);
}
#endif  // !defined(_WIN32)

void TestHandshake::checkAuthValuesEIP8(uint64_t _expectedRemoteVersion)
{
    EXPECT_EQ(m_remote, Public("fda1cff674c90c9a197539fe3dfb53086ace64f83ed7c6eabec741f7f381cc803e5"
                               "2ab2cd55d5569bce4347107a310dfd5f88a010cd2ffd1005ca406f1842877"));
    EXPECT_EQ(
        m_remoteNonce, h256("7e968bba13b6c50e2c4cd7f241cc0d64d1ac25c7f5952df231ac6a2bda8ee5d6"));
    EXPECT_EQ(
        m_ecdheRemote, Public("654d1044b69c577a44e5f01a1209523adb4026e70c62d1c13a067acabc09d2667a49"
                              "821a0ad4b634554d330a15a58fe61f8a8e0544b310c6de7b0c8da7528a8d"));
    EXPECT_EQ(m_remoteVersion, _expectedRemoteVersion);
}

void TestHandshake::checkAckValuesEIP8(uint64_t _expectedRemoteVersion)
{
    EXPECT_EQ(
        m_remoteNonce, h256("559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd"));
    EXPECT_EQ(
        m_ecdheRemote, Public("b6d82fa3409da933dbf9cb0140c5dde89f4e64aec88d476af648880f4a10e1e49fe3"
                              "5ef3e69e93dd300b4797765a747c6384a6ecf5db9c2690398607a86181e4"));
    EXPECT_EQ(m_remoteVersion, _expectedRemoteVersion);
}

#define throwErrorCode(what, error)                                                      \
    {                                                                                    \
        if (error)                                                                       \
            BOOST_THROW_EXCEPTION(Exception() << errinfo_comment(what + _ec.message())); \
    }

shared_ptr<TestHandshake> TestHandshake::runWithInput(
    Secret _hostAlias, bytes _packet, NodeID _remoteID)
{
    // Spawn a listener which sends the packet to any client.
    ba::io_service io;
    bi::tcp::acceptor acceptor(io);
    bi::tcp::endpoint endpoint(bi::address::from_string("127.0.0.1"), 0);
    acceptor.open(endpoint.protocol());
    acceptor.bind(endpoint);
    acceptor.listen();
    auto server = std::make_shared<RLPXSocket>(io);
    acceptor.async_accept(server->ref(), [_packet, server](boost::system::error_code const& _ec) {
        throwErrorCode("accept error: ", _ec);
        ba::async_write(server->ref(), ba::buffer(_packet),
            [](const boost::system::error_code& _ec, std::size_t) {
                throwErrorCode("write error: ", _ec);
            });
    });

    // Spawn a client to execute the handshake.
    auto host = make_shared<Host>("peer name",
        make_pair(_hostAlias, IdentitySchemeV4::createENR(_hostAlias, endpoint.address(), 0, 0)));
    auto client = make_shared<RLPXSocket>(io);
    shared_ptr<TestHandshake> handshake;
    if (_remoteID == NodeID())
        handshake.reset(new TestHandshake(host.get(), client));
    else
        handshake.reset(new TestHandshake(host.get(), client, _remoteID));

    client->ref().async_connect(
        acceptor.local_endpoint(), [handshake](boost::system::error_code const& _ec) {
            throwErrorCode("connect error: ", _ec);
            handshake->start();
        });

    // Run all I/O to completion and return the finished handshake.
    io.run();
    return handshake;
}

void TestHandshake::transition(boost::system::error_code _ec)
{
    if (!_ec && m_nextState == ReadHello)
    {
        cancel();
        return;
    }
    RLPXHandshake::transition(_ec);
}

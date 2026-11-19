//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_sha256.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests Sha256 against the FIPS 180-4 test vectors and Qt's QCryptographicHash.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/sha256.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCryptographicHash>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

std::string qtHex(const std::vector<std::byte>& data)
{
    const QByteArray bytes(reinterpret_cast<const char*>(data.data()), qsizetype(data.size()));
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex().toStdString();
}

} // namespace

//=============================================================================================================
/**
 * @brief Tests Sha256.
 */
class TestBaseSha256 : public QObject
{
    Q_OBJECT

private slots:
    void hashesTheStandardVectors();
    void matchesQtForEveryLengthAndSplit();
    void digestDoesNotEndTheHash();
};

//=============================================================================================================

void TestBaseSha256::hashesTheStandardVectors()
{
    QCOMPARE(to_hex(Sha256::hash("")), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    QCOMPARE(to_hex(Sha256::hash("abc")), std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    QCOMPARE(to_hex(Sha256::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")),
             std::string("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
    QCOMPARE(to_hex(Sha256::hash(std::string(1'000'000, 'a'))),
             std::string("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
}

//=============================================================================================================

void TestBaseSha256::matchesQtForEveryLengthAndSplit()
{
    std::vector<std::byte> data(300);
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<std::byte>((i * 167) + (i >> 3)); // every byte value, no runs

    const std::span<const std::byte> all(data);
    for (std::size_t n = 0; n <= data.size(); ++n) {
        const std::span<const std::byte> part = all.first(n);
        const std::string expected = qtHex({part.begin(), part.end()});
        QCOMPARE(to_hex(Sha256::hash(part)), expected);
        const std::size_t cut = n / 3;
        Sha256 sha;
        sha.update(part.first(cut)).update(part.subspan(cut, (n - cut) / 2)).update(part.subspan(cut + (n - cut) / 2));
        QCOMPARE(to_hex(sha.digest()), expected);
    }
}

//=============================================================================================================

void TestBaseSha256::digestDoesNotEndTheHash()
{
    Sha256 sha;
    sha.update("ab");
    QCOMPARE(sha.digest(), Sha256::hash("ab"));
    sha.update("c");
    QCOMPARE(sha.digest(), Sha256::hash("abc"));
    sha.reset();
    QCOMPARE(sha.digest(), Sha256::hash(""));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestBaseSha256)
#include "test_base_sha256.moc"

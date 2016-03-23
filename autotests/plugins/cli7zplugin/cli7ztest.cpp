/*
  * Copyright (c) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cli7ztest.h"

#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

#include <KPluginLoader>

QTEST_GUILESS_MAIN(Cli7zTest)

using namespace Kerfuffle;

void Cli7zTest::initTestCase()
{
    qRegisterMetaType<ArchiveEntry>();

    const auto plugins = KPluginLoader::findPluginsById(QStringLiteral("kerfuffle"), QStringLiteral("kerfuffle_cli7z"));
    if (plugins.size() == 1) {
        m_pluginMetadata = plugins.at(0);
    }
}

void Cli7zTest::testArchive_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedFileName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QString>("expectedSubfolderName");

    QString archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("archive with one top-level folder")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << false << true << Archive::Unencrypted
            << QStringLiteral("A");
}

void Cli7zTest::testArchive()
{
    if (!m_pluginMetadata.isValid()) {
        QSKIP("Could not find the cli7z plugin. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    Archive *archive = Archive::create(archivePath, m_pluginMetadata, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the cli7z plugin. Skipping test.", SkipSingle);
    }

    QFETCH(QString, expectedFileName);
    QCOMPARE(QFileInfo(archive->fileName()).fileName(), expectedFileName);

    QFETCH(bool, isReadOnly);
    QCOMPARE(archive->isReadOnly(), isReadOnly);

    QFETCH(bool, isSingleFolder);
    QCOMPARE(archive->isSingleFolderArchive(), isSingleFolder);

    QFETCH(Archive::EncryptionType, expectedEncryptionType);
    QCOMPARE(archive->encryptionType(), expectedEncryptionType);

    QFETCH(QString, expectedSubfolderName);
    QCOMPARE(archive->subfolderName(), expectedSubfolderName);
}

void Cli7zTest::testList_data()
{
    QTest::addColumn<QString>("outputTextFile");
    QTest::addColumn<int>("expectedEntriesCount");
    // Index of some entry to be tested.
    QTest::addColumn<int>("someEntryIndex");
    // Entry metadata.
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<bool>("isDirectory");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<qulonglong>("expectedSize");
    QTest::addColumn<QString>("expectedTimestamp");

    // p7zip version 15.14 tests

    QTest::newRow("normal-file-1514")
            << QFINDTESTDATA("data/archive-with-symlink-1514.txt") << 10
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-1514")
            << QFINDTESTDATA("data/archive-encrypted-1514.txt") << 9
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    // p7zip version 15.09 tests

    QTest::newRow("normal-file-1509")
            << QFINDTESTDATA("data/archive-with-symlink-1509.txt") << 10
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-1509")
            << QFINDTESTDATA("data/archive-encrypted-1509.txt") << 9
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    // p7zip version 9.38.1 tests

    QTest::newRow("normal-file-9381")
            << QFINDTESTDATA("data/archive-with-symlink-9381.txt") << 10
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-9381")
            << QFINDTESTDATA("data/archive-encrypted-9381.txt") << 9
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");
}

void Cli7zTest::testList()
{
    CliPlugin *plugin = new CliPlugin(this, {QStringLiteral("dummy.7z")});
    QSignalSpy signalSpy(plugin, SIGNAL(entry(ArchiveEntry)));

    QFETCH(QString, outputTextFile);
    QFETCH(int, expectedEntriesCount);

    QFile outputText(outputTextFile);
    QVERIFY(outputText.open(QIODevice::ReadOnly));

    QTextStream outputStream(&outputText);
    while (!outputStream.atEnd()) {
        const QString line(outputStream.readLine());
        QVERIFY(plugin->readListLine(line));
    }

    QCOMPARE(signalSpy.count(), expectedEntriesCount);

    QFETCH(int, someEntryIndex);
    QVERIFY(someEntryIndex < signalSpy.count());
    ArchiveEntry entry = qvariant_cast<ArchiveEntry>(signalSpy.at(someEntryIndex).at(0));

    QFETCH(QString, expectedName);
    QCOMPARE(entry[FileName].toString(), expectedName);

    QFETCH(bool, isDirectory);
    QCOMPARE(entry[IsDirectory].toBool(), isDirectory);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(entry[IsPasswordProtected].toBool(), isPasswordProtected);

    QFETCH(qulonglong, expectedSize);
    QCOMPARE(entry[Size].toULongLong(), expectedSize);

    QFETCH(QString, expectedTimestamp);
    QCOMPARE(entry[Timestamp].toString(), expectedTimestamp);

    plugin->deleteLater();
}
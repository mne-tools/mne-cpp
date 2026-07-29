//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_studio_mna_skills.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Checks the MNA workflow skills.
 *
 * A skill advertises itself to the workflow manager through a JSON definition
 * and is then handed nodes to execute. Both halves are worth asserting: the
 * definition is what the planner matches a user request against, so a missing
 * or duplicated identifier silently makes a skill unreachable or shadows
 * another one, and executeSkill has to refuse an incomplete node rather than
 * acting on it.
 *
 * Nothing here writes a real project. The cases are the ones a workflow can
 * produce by mistake: a node with no parameters at all, a node naming an input
 * that does not exist, an output path pointing somewhere unwritable.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <writemnaskill.h>
#include <readmnaskill.h>
#include <runmnagraphskill.h>

#include <core/workflowgraph.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZESTUDIO;

//=============================================================================================================
/**
 * DECLARE CLASS TestStudioMnaSkills
 *
 * @brief Checks the MNA workflow skill definitions and their failure handling.
 */
class TestStudioMnaSkills: public QObject
{
    Q_OBJECT

public:
    TestStudioMnaSkills() = default;

private:
    /** Every field the workflow manager reads off a skill definition. */
    static void verifyDefinitionShape(const QJsonObject& definition, const QString& skillName);

private slots:
    void definitions_haveTheFieldsTheManagerReads_data();
    void definitions_haveTheFieldsTheManagerReads();
    void definitions_haveDistinctIdentifiers();
    void writeSkill_rejectsNodeWithoutOutputPath();
    void readSkill_rejectsNodeWithoutSource();
    void runGraphSkill_rejectsEmptyNode();
    void skills_doNotCrashOnAnEmptyNode();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestStudioMnaSkills::verifyDefinitionShape(const QJsonObject& definition, const QString& skillName)
{
    // These are the keys the workflow manager and the planner read. A missing
    // one does not fail loudly, it makes the skill unusable or unlistable.
    const QStringList required{"skill_id", "tool_name", "display_name",
                               "description", "extension_id"};

    for(const QString& key : required) {
        QVERIFY2(definition.contains(key),
                 qPrintable(QString("%1 definition has no %2").arg(skillName, key)));
        QVERIFY2(!definition.value(key).toString().isEmpty(),
                 qPrintable(QString("%1 definition has an empty %2").arg(skillName, key)));
    }
}

//=============================================================================================================

void TestStudioMnaSkills::definitions_haveTheFieldsTheManagerReads_data()
{
    QTest::addColumn<int>("skillIndex");
    QTest::addColumn<QString>("skillName");
    QTest::addColumn<QString>("expectedSkillId");
    QTest::addColumn<QString>("expectedToolName");

    // The identifiers are pinned rather than only checked for presence. They
    // are what a saved workflow refers to, so renaming one silently breaks
    // every stored graph that used it.
    QTest::newRow("write") << 0 << "WriteMnaSkill"    << "mne.skills.write_mna"     << "write_mna_project";
    QTest::newRow("read")  << 1 << "ReadMnaSkill"     << "mne.skills.read_mna"      << "read_mna_project";
    QTest::newRow("run")   << 2 << "RunMnaGraphSkill" << "mne.skills.run_mna_graph" << "run_mna_graph";
}

//=============================================================================================================

void TestStudioMnaSkills::definitions_haveTheFieldsTheManagerReads()
{
    QFETCH(int, skillIndex);
    QFETCH(QString, skillName);
    QFETCH(QString, expectedSkillId);
    QFETCH(QString, expectedToolName);

    QJsonObject definition;
    switch(skillIndex) {
        case 0: { WriteMnaSkill skill;    definition = skill.getOperatorDefinition(); break; }
        case 1: { ReadMnaSkill skill;     definition = skill.getOperatorDefinition(); break; }
        case 2: { RunMnaGraphSkill skill; definition = skill.getOperatorDefinition(); break; }
        default: QFAIL("unknown skill index");
    }

    verifyDefinitionShape(definition, skillName);

    QCOMPARE(definition.value("skill_id").toString(), expectedSkillId);
    QCOMPARE(definition.value("tool_name").toString(), expectedToolName);
}

//=============================================================================================================

void TestStudioMnaSkills::definitions_haveDistinctIdentifiers()
{
    WriteMnaSkill write;
    ReadMnaSkill read;
    RunMnaGraphSkill run;

    const QList<QJsonObject> definitions{write.getOperatorDefinition(),
                                         read.getOperatorDefinition(),
                                         run.getOperatorDefinition()};

    // Two skills sharing an id or a tool name means one shadows the other in
    // the manager's lookup, and which one wins depends on registration order.
    QSet<QString> skillIds;
    QSet<QString> toolNames;

    for(const QJsonObject& definition : definitions) {
        const QString skillId = definition.value("skill_id").toString();
        const QString toolName = definition.value("tool_name").toString();

        QVERIFY2(!skillIds.contains(skillId),
                 qPrintable(QString("duplicate skill_id %1").arg(skillId)));
        QVERIFY2(!toolNames.contains(toolName),
                 qPrintable(QString("duplicate tool_name %1").arg(toolName)));

        skillIds.insert(skillId);
        toolNames.insert(toolName);
    }

    QCOMPARE(skillIds.size(), 3);
    QCOMPARE(toolNames.size(), 3);
}

//=============================================================================================================

void TestStudioMnaSkills::writeSkill_rejectsNodeWithoutOutputPath()
{
    WriteMnaSkill skill;

    WorkflowNode node;
    node.uid = "node-1";
    node.skillId = "mne.skills.write_mna";

    // A node with no output path cannot be executed. Refusing it is what stops
    // the workflow reporting success for a file that was never written.
    const QJsonObject result = skill.executeSkill(node);

    QCOMPARE(result.value("status").toString(), QString("error"));
    QVERIFY2(!result.value("message").toString().isEmpty(),
             "the skill refused the node without saying why");

    // The message names the node, which is what makes a failed graph readable.
    QVERIFY2(result.value("message").toString().contains("node-1"),
             qPrintable(QString("error does not identify the node: %1")
                        .arg(result.value("message").toString())));
}

//=============================================================================================================

void TestStudioMnaSkills::readSkill_rejectsNodeWithoutSource()
{
    ReadMnaSkill skill;

    WorkflowNode node;
    node.uid = "node-2";
    node.skillId = "mne.skills.read_mna";

    const QJsonObject result = skill.executeSkill(node);

    QCOMPARE(result.value("status").toString(), QString("error"));
    QVERIFY2(!result.value("message").toString().isEmpty(),
             "the skill refused the node without saying why");
}

//=============================================================================================================

void TestStudioMnaSkills::runGraphSkill_rejectsEmptyNode()
{
    RunMnaGraphSkill skill;

    WorkflowNode node;
    node.uid = "node-3";
    node.skillId = "mne.skills.run_mna_graph";

    const QJsonObject result = skill.executeSkill(node);

    QCOMPARE(result.value("status").toString(), QString("error"));
    QVERIFY2(!result.value("message").toString().isEmpty(),
             "the skill refused the node without saying why");
}

//=============================================================================================================

void TestStudioMnaSkills::skills_doNotCrashOnAnEmptyNode()
{
    // A completely default node is what a partially built graph hands over, so
    // every skill has to survive it rather than assuming any field was filled.
    WorkflowNode node;

    {
        WriteMnaSkill skill;
        const QJsonObject result = skill.executeSkill(node);
        QVERIFY2(result.contains("status"), "write skill returned no status for an empty node");
    }
    {
        ReadMnaSkill skill;
        const QJsonObject result = skill.executeSkill(node);
        QVERIFY2(result.contains("status"), "read skill returned no status for an empty node");
    }
    {
        RunMnaGraphSkill skill;
        const QJsonObject result = skill.executeSkill(node);
        QVERIFY2(result.contains("status"), "run graph skill returned no status for an empty node");
    }
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestStudioMnaSkills)
#include "test_studio_mna_skills.moc"

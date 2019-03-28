#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "query/frontend/semantic/required_privileges.hpp"
#include "storage/common/types/types.hpp"

#include "query_common.hpp"

using namespace query;

class FakeDbAccessor {};

const std::string EDGE_TYPE = "0";
const std::string LABEL_0 = "label0";
const std::string LABEL_1 = "label1";
const std::string PROP_0 = "prop0";

using ::testing::UnorderedElementsAre;

class TestPrivilegeExtractor : public ::testing::Test {
 protected:
  AstStorage storage;
  FakeDbAccessor dba;
};

TEST_F(TestPrivilegeExtractor, CreateNode) {
  auto *query = QUERY(SINGLE_QUERY(CREATE(PATTERN(NODE("n")))));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::CREATE));
}

TEST_F(TestPrivilegeExtractor, MatchNodeDelete) {
  auto *query =
      QUERY(SINGLE_QUERY(MATCH(PATTERN(NODE("n"))), DELETE(IDENT("n"))));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::MATCH,
                                   AuthQuery::Privilege::DELETE));
}

TEST_F(TestPrivilegeExtractor, MatchNodeReturn) {
  auto *query = QUERY(SINGLE_QUERY(MATCH(PATTERN(NODE("n"))), RETURN("n")));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::MATCH));
}

TEST_F(TestPrivilegeExtractor, MatchCreateExpand) {
  auto *query = QUERY(SINGLE_QUERY(
      MATCH(PATTERN(NODE("n"))),
      CREATE(PATTERN(NODE("n"),
                     EDGE("r", EdgeAtom::Direction::OUT, {EDGE_TYPE}),
                     NODE("m")))));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::MATCH,
                                   AuthQuery::Privilege::CREATE));
}

TEST_F(TestPrivilegeExtractor, MatchNodeSetLabels) {
  auto *query = QUERY(
      SINGLE_QUERY(MATCH(PATTERN(NODE("n"))), SET("n", {LABEL_0, LABEL_1})));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::MATCH,
                                   AuthQuery::Privilege::SET));
}

TEST_F(TestPrivilegeExtractor, MatchNodeSetProperty) {
  auto *query = QUERY(
      SINGLE_QUERY(MATCH(PATTERN(NODE("n"))),
                   SET(PROPERTY_LOOKUP(storage.Create<Identifier>("n"), PROP_0),
                       LITERAL(42))));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::MATCH,
                                   AuthQuery::Privilege::SET));
}

TEST_F(TestPrivilegeExtractor, MatchNodeSetProperties) {
  auto *query =
      QUERY(SINGLE_QUERY(MATCH(PATTERN(NODE("n"))), SET("n", LIST())));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::MATCH,
                                   AuthQuery::Privilege::SET));
}

TEST_F(TestPrivilegeExtractor, MatchNodeRemoveLabels) {
  auto *query = QUERY(
      SINGLE_QUERY(MATCH(PATTERN(NODE("n"))), REMOVE("n", {LABEL_0, LABEL_1})));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::MATCH,
                                   AuthQuery::Privilege::REMOVE));
}

TEST_F(TestPrivilegeExtractor, MatchNodeRemoveProperty) {
  auto *query = QUERY(SINGLE_QUERY(
      MATCH(PATTERN(NODE("n"))),
      REMOVE(PROPERTY_LOOKUP(storage.Create<Identifier>("n"), PROP_0))));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::MATCH,
                                   AuthQuery::Privilege::REMOVE));
}

TEST_F(TestPrivilegeExtractor, CreateIndex) {
  auto *query = CREATE_INDEX_ON(storage.GetLabelIx(LABEL_0),
                                storage.GetPropertyIx(PROP_0));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::INDEX));
}

TEST_F(TestPrivilegeExtractor, AuthQuery) {
  auto *query = AUTH_QUERY(AuthQuery::Action::CREATE_ROLE, "", "role", "",
                           nullptr, std::vector<AuthQuery::Privilege>{});
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::AUTH));
}

TEST_F(TestPrivilegeExtractor, StreamQuery) {
  std::string stream_name("kafka");
  std::string stream_uri("localhost:1234");
  std::string stream_topic("tropik");
  std::string transform_uri("localhost:1234/file.py");

  std::vector<StreamQuery *> stream_queries = {
      CREATE_STREAM(stream_name, stream_uri, stream_topic, transform_uri,
                    nullptr, nullptr),
      DROP_STREAM(stream_name),
      SHOW_STREAMS,
      START_STREAM(stream_name, nullptr),
      STOP_STREAM(stream_name),
      START_ALL_STREAMS,
      STOP_ALL_STREAMS};

  for (auto *query : stream_queries) {
    EXPECT_THAT(GetRequiredPrivileges(query),
                UnorderedElementsAre(AuthQuery::Privilege::STREAM));
  }
}

TEST_F(TestPrivilegeExtractor, ShowIndexInfo) {
  auto *query = storage.Create<InfoQuery>();
  query->info_type_ = InfoQuery::InfoType::INDEX;
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::INDEX));
}

TEST_F(TestPrivilegeExtractor, ShowStatsInfo) {
  auto *query = storage.Create<InfoQuery>();
  query->info_type_ = InfoQuery::InfoType::STORAGE;
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::STATS));
}

TEST_F(TestPrivilegeExtractor, ShowConstraintInfo) {
  auto *query = storage.Create<InfoQuery>();
  query->info_type_ = InfoQuery::InfoType::CONSTRAINT;
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::CONSTRAINT));
}

TEST_F(TestPrivilegeExtractor, CreateConstraint) {
  auto *query = storage.Create<ConstraintQuery>();
  query->action_type_ = ConstraintQuery::ActionType::CREATE;
  query->constraint_.label = storage.GetLabelIx("label");
  query->constraint_.properties.push_back(storage.GetPropertyIx("prop0"));
  query->constraint_.properties.push_back(storage.GetPropertyIx("prop1"));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::CONSTRAINT));
}

TEST_F(TestPrivilegeExtractor, DropConstraint) {
  auto *query = storage.Create<ConstraintQuery>();
  query->action_type_ = ConstraintQuery::ActionType::DROP;
  query->constraint_.label = storage.GetLabelIx("label");
  query->constraint_.properties.push_back(storage.GetPropertyIx("prop0"));
  query->constraint_.properties.push_back(storage.GetPropertyIx("prop1"));
  EXPECT_THAT(GetRequiredPrivileges(query),
              UnorderedElementsAre(AuthQuery::Privilege::CONSTRAINT));
}

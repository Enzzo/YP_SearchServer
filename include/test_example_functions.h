#pragma once

#include "search_server.h"
#include "remove_duplicates.h"

#include <iomanip>

template <typename Func>
void RunTestImpl(Func, const std::string&);

void AssertImpl(bool,const std::string&, const std::string&, const std::string&, const unsigned, const std::string&);

template <typename T, typename U>
void AssertEqualImpl(const T&, const U&, const std::string&, const std::string&, const std::string&,
    const std::string&, unsigned, const std::string&);

//macros for testing
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__,  hint)
#define ASSERT(expr)  AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__,  "")

#define ASSERT_EQUAL_HINT(left, right, hint) AssertEqualImpl((left), (right), #left, #right, __FILE__, __FUNCTION__, __LINE__, hint)
#define ASSERT_EQUAL(left, right) ASSERT_EQUAL_HINT(left, right, "");

#define RUN_TEST(func)  RunTestImpl((func), #func)

#define TEST TestSearchServer();

bool is_equal(const double l, const double r);

// -------- Start of search engine unit tests ----------

//creating an instance of a search server that covers all the tests
SearchServer GetTestServer();

SearchServer GetTestServerWithDuplicates();

void SearchServer_AddDocument_CheckSize_SizeChange();

void SearchServer_AddDocument_CheckSize_SizeEmpty();

void SearchServer_AddDocument_CheckDocumentsCount_Equal();

void TestStopWords();

void TestMinusWords();

void TestMatchingDocuments();

void TestByRelevance();

void TestByRelevance_MinusWords();

void TestOfRating();

void TestSortingByPredicate();

void SearchServer_Status_CheckSize_SizeChange();

void SearchServer_Status_CheckSize_SizeEmpty();

void SearchServer_Status_CheckId_IdFound();

void SearchServer_Status_CheckDocumentsCount_Equal();

void TestIterators();

void TestGetWordFrequencies();

void TestRemoveDocuments();

void TestRemoveDuplicates();

// The TestSearchServer function is the entry point for running tests
void TestSearchServer();
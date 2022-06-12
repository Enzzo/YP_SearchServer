#pragma once

#include <cmath>
#include <math.h>
#include <set>
#include <map>
#include <algorithm>
#include <execution>
#include <future>
#include <mutex>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

using namespace std::literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    std::set<std::string> stop_words_;
    std::map<int, std::map<std::string, double>> doc_to_word_freqs_;
    std::map<std::string, std::map<int, double>> word_to_doc_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_id_;


public:
    // Defines an invalid document id
    // You can refer this constant as SearchServer::INVALID_DOCUMENT_ID
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    explicit SearchServer() = default;

    explicit SearchServer(const std::string_view&);

    template <typename StringContainer>
    explicit SearchServer(const StringContainer&);

    explicit SearchServer(const std::string&);



    void AddDocument(int, const std::string_view&, DocumentStatus, const std::vector<int>&);

    inline int GetDocumentCount() const noexcept {
        return documents_.size();
    }

    //O(1)
    inline std::set<int>::const_iterator begin() const noexcept {
        return document_id_.begin();
    }

    //O(1)
    inline std::set<int>::const_iterator end() const noexcept {
        return document_id_.end();
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view&, DocumentStatus) const;
    std::vector<Document> FindTopDocuments(const std::string_view&) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&&, const std::string_view&, DocumentPredicate) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&&, const std::string_view&, DocumentStatus) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&&, const std::string_view&) const;

    template<typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy&&, const std::string_view&, int) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view&, int) const;

    const std::map<std::string_view, double>& GetWordFrequencies(const int) const noexcept;

    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&&, const int document_id);
    void RemoveDocument(const int document_id);

private:

    template<typename str>
    static bool IsValidWord(const str&);

    static int ComputeAverageRating(const std::vector<int>&);

    inline bool IsStopWord(const std::string_view& word) const {
        return stop_words_.count(static_cast<std::string>(word)) > 0;
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string_view&) const;

    QueryWord ParseQueryWord(std::string) const;

    Query ParseQuery(const std::string_view&) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string&) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&&, const Query&, DocumentPredicate) const;

    template <typename StringContainer>
    void CheckValidity(const StringContainer&);

    template <typename StringContainer>
    std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer&);
};

void RemoveDuplicates(SearchServer&);

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
    CheckValidity(stop_words);
    stop_words_ = MakeUniqueNonEmptyStrings(stop_words);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const {

    std::vector<Document> result;
    Query query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    result.swap(matched_documents);
    return result;
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentStatus status) const {

    return FindTopDocuments(
        policy,
        raw_query,
        [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [this, &document_predicate, &document_to_relevance](const std::string& word) {
        if (this->word_to_doc_freqs_.count(word) != 0) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_doc_freqs_.at(word)) {
                const auto& document_data = this->documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        });

    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [this, &document_to_relevance](const std::string& word) {
        if (this->word_to_doc_freqs_.count(word) != 0) {
            for (const auto [document_id, _] : this->word_to_doc_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        });

    std::vector<Document> matched_documents;
    matched_documents.reserve(document_to_relevance.size());

    auto pred = [&matched_documents, this](const std::pair<int, double>& p) {
        matched_documents.push_back({ p.first, p.second, this->documents_.at(p.first).rating });
    };
    std::for_each(policy, document_to_relevance.begin(), document_to_relevance.end(), pred);

    return matched_documents;
}

template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
    if (doc_to_word_freqs_.count(document_id)) {

        this->doc_to_word_freqs_.erase(document_id);
        this->documents_.erase(document_id);

        auto it = std::lower_bound(document_id_.begin(), document_id_.end(), document_id);
        std::future<void> f3 = std::async([it, this] {this->document_id_.erase(it); });

        std::for_each(policy, word_to_doc_freqs_.begin(), word_to_doc_freqs_.end(), [document_id](auto& word) {
            if (word.second.find(document_id) != word.second.end()) {
                word.second.erase(document_id);
            }
            });
    }
}

template<typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy&& policy, const std::string_view& raw_query, int document_id) const {

    Query query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;
    const std::map<std::string, double>& doc = doc_to_word_freqs_.at(document_id);
    
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&matched_words, &doc](const std::string& word) {
        auto it = doc.find(word);
        if (it != doc.end())
            matched_words.push_back(std::string_view(it->first));
        });

    if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [&doc](std::string word) {
        return doc.count(word);
        })){
        matched_words.clear();
    }

    return { matched_words, documents_.at(document_id).status };
}

template <typename StringContainer>
void SearchServer::CheckValidity(const StringContainer& strings) {
    for (const auto& str : strings) {
        if (!IsValidWord(str)) {
            throw std::invalid_argument("invalid stop-words in constructor"s);
        }
    }
}

template <typename StringContainer>
std::set<std::string> SearchServer::MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const auto& str : strings) {
        non_empty_strings.emplace(str);
    }
    return non_empty_strings;
}

template<typename str>
bool SearchServer::IsValidWord(const str& word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}


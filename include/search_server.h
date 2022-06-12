#pragma once

#include <cmath>
#include <math.h>
#include <set>
#include <map>
#include <algorithm>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

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

    std::map<int, DocumentData> documents_;
    
    std::set<int> document_id_;

public:
    // Defines an invalid document id
    // You can refer this constant as SearchServer::INVALID_DOCUMENT_ID
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer&);

    explicit SearchServer(const std::string&);

    void AddDocument(int, const std::string&, DocumentStatus, const std::vector<int>&);

    inline int GetDocumentCount() const noexcept{
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
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string&, DocumentStatus) const;

    std::vector<Document> FindTopDocuments(const std::string&) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string&, int) const;

    //O(log N)
    const std::map<std::string, double>& GetWordFrequencies(const int) const noexcept;

    //O(W log N)
    void RemoveDocument(int document_id);

private:

    static bool IsValidWord(const std::string&);

    static int ComputeAverageRating(const std::vector<int>&);

    inline bool IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

    [[nodiscard]] bool SplitIntoWordsNoStop(const std::string&, std::vector<std::string>&) const;

    [[nodiscard]] bool ParseQueryWord(std::string, QueryWord&) const;

    [[nodiscard]] bool ParseQuery(const std::string&, Query&) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string&) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query&, DocumentPredicate) const;

    template <typename StringContainer>
    void CheckValidity(const StringContainer&);

    template <typename StringContainer>
    std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer&);
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
    CheckValidity(stop_words);
    stop_words_ = MakeUniqueNonEmptyStrings(stop_words);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {

    std::vector<Document> result;
    Query query;
    if (!ParseQuery(raw_query, query)) {
        throw std::invalid_argument("invalid request");
    }
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
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

    // Exchange matched_documents and result instead of deep copying
    result.swap(matched_documents);
    return result;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;

    for (const std::string& word : query.plus_words) {
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

        for (const auto [document_id, word_freq] : doc_to_word_freqs_) {

            const auto& document_data = documents_.at(document_id);

            for (const auto [w, fr] : word_freq) {
                if (w == word) {
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id] += fr * inverse_document_freq;
                    }
                    continue;
                }
            }
        }
    }


    for (const std::string& word : query.minus_words) {
        for (const auto& [id, doc] : doc_to_word_freqs_) {
            if (document_to_relevance.count(id)) {
                for (const auto& [w, fr] : doc) {
                    if (w == word) {
                        document_to_relevance.erase(id);
                        continue;
                    }
                }
            }
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename StringContainer>
void SearchServer::CheckValidity(const StringContainer& strings) {
    for (const std::string& str : strings) {
        if (!IsValidWord(str)) {
            throw std::invalid_argument("invalid stop-words in constructor"s);
        }
    }
}

template <typename StringContainer>
std::set<std::string> SearchServer::MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
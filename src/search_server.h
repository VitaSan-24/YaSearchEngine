#pragma once
/*
 * search_server.h
 *
 *  Created on: 7 сент. 2024 г.
 *      Author: vitasan
 */

#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <algorithm>
#include <set>
#include <stdexcept>
#include "document.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

class SearchServer {
public:

    SearchServer();
    template<typename Container>
    explicit SearchServer(const Container &container);
    explicit SearchServer(const std::string &text_stop_words);

    int GetDocumentCount() const;

    std::vector<std::string> SplitIntoWords(const std::string &text) const;

    void AddDocument(int document_id, const std::string &document,
            DocumentStatus status, const std::vector<int> &rating);

    template<typename Filter>
    std::vector<Document> FindTopDocuments(const std::string &raw_query,
            Filter filter_fun) const;

    std::vector<Document> FindTopDocuments(const std::string &raw_query) const;

    std::vector<Document> FindTopDocuments(const std::string &raw_query,
            DocumentStatus find_status) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
            const std::string &raw_query, int document_id) const;

    int GetDocumentId(int index) const;

private:

    struct DocumentProperties {
        int rating;
        DocumentStatus status;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::vector<std::string> minus_words;
    };

    std::vector<int> insert_doc_;
    std::map<int, DocumentProperties> properties_documents_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    std::set<std::string> stop_words_;

    DocumentProperties GetPropertiesDocument(const int &id) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);
    bool IsStopWord(const std::string &word) const;
    std::vector<std::string> SplitIntoWordsNoStop(
            const std::string &text) const;
    static bool IsValidString(const std::string &str);
    void PossibleAddDocument(int document_id,
            const std::string &document) const;
    void ParseQuery(const std::string &text, Query &query) const;
    void CheckQurey(Query &query) const;
    double CalcIDF(const std::string &word) const;

    template<typename FilterFun>
    std::vector<Document> FindAllDocuments(const Query &query,
            FilterFun lambda_func) const;
};

template<typename Filter>
std::vector<Document> SearchServer::FindTopDocuments(
        const std::string &raw_query, Filter filter_fun) const {
    std::vector<Document> result;
    std::set<std::string> query_words;
    Query query;
    ParseQuery(raw_query, query);
    CheckQurey(query);
    result = FindAllDocuments(query, filter_fun);

    auto by_relevance = [](const Document &lhs, const Document &rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        }
        return lhs.relevance > rhs.relevance;
    };

    std::sort(result.begin(), result.end(), by_relevance);
    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return result;
}

template<typename FilterFun>
std::vector<Document> SearchServer::FindAllDocuments(const Query &query,
        FilterFun lambda_func) const {
    std::vector<Document> matched_documents;
    std::map<int, double> query_result;

    if (query.plus_words.size() != 0) {
        for (const std::string &plus_word : query.plus_words) {
            const auto &temp_map = word_to_document_freqs_.find(plus_word);
            if (temp_map != word_to_document_freqs_.end()) {
                const std::map<int, double> &temp_set = temp_map->second;
                double idf = CalcIDF(plus_word);
                for (auto &element : temp_set) {
                    double &rel = query_result[element.first];
                    rel = rel + idf * element.second;
                }
            }
        }
        if (query.minus_words.size() != 0) {
            for (const std::string &minus_word : query.minus_words) {
                const auto &temp_map = word_to_document_freqs_.find(minus_word);
                if (temp_map != word_to_document_freqs_.end()) {
                    const std::map<int, double> &temp_set = temp_map->second;
                    for (auto &element : temp_set) {
                        query_result.erase(element.first);
                    }
                }
            }
        }
        for (auto &res : query_result) {
            SearchServer::DocumentProperties doc_prop = GetPropertiesDocument(
                    res.first);
            if (lambda_func(res.first, doc_prop.status, doc_prop.rating)) {
                matched_documents.push_back( // @suppress("Invalid arguments")
                        { res.first, res.second, doc_prop.rating });
            }
        }
    }
    return matched_documents;
}

template<typename Container>
SearchServer::SearchServer(const Container &container) {

    for (const std::string &word : container) {

        if (!IsValidString(word)) {
            throw std::invalid_argument(
                    "Слово `" + word + "` имеет запрещенные символы.");
            std::vector<std::string> words;
        }

        const auto &result = find(begin(stop_words_), end(stop_words_), word);
        if (result == stop_words_.end())
            if (!word.empty()) {
                stop_words_.insert(word);
            }
    }
}


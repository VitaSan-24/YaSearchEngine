/*
 * search_server.h
 *
 *  Created on: 7 сент. 2024 г.
 *      Author: vitasan
 */

#pragma once
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <set>
#include "document.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;


class SearchServer {
public:

    explicit SearchServer(const std::string &text_stop_words);
    template<typename Container>
    explicit SearchServer(const Container &container);

    int GetDocumentCount() const;

    std::vector<std::string> SplitIntoWords(const std::string &text) const;

    void AddDocument(int document_id, const  std::string &document,
            DocumentStatus status, const  std::vector<int> &rating);

    std::vector<Document> FindTopDocuments(const  std::string &raw_query) const;

    std::vector<Document> FindTopDocuments(const  std::string &raw_query,
            DocumentStatus find_status) const;

    template<typename Filter>
    std::vector<Document> FindTopDocuments(const std::string &raw_query,
            Filter filter_fun) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const  std::string &raw_query,
            int document_id) const;

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
    std::map<std::string,  std::map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    std::set<std::string> stop_words_;

    DocumentProperties GetPropertiesDocument(const int &id) const;

    static int ComputeAverageRating(const  std::vector<int> &ratings);
    bool IsStopWord(const  std::string &word) const;
    std::vector<std::string> SplitIntoWordsNoStop(const  std::string &text) const;
    static bool IsValidString(const  std::string &str);
    void PossibleAddDocument(int document_id, const  std::string &document) const;
    void ParseQuery(const  std::string &text, Query &query) const;
    void CheckQurey(Query &query) const;
    double CalcIDF(const  std::string &word) const;

    template<typename FilterFun>
    std::vector<Document> FindAllDocuments(const Query &query,
            FilterFun lambda_func) const;
};


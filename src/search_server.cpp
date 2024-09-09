/*
 * search_server.cpp
 *
 *  Created on: 7 сент. 2024 г.
 *      Author: vitasan
 */
#include "search_server.h"
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <set>
#include <stdexcept>

using namespace std;

SearchServer::SearchServer() {
}
;

SearchServer::SearchServer(const std::string &stop_words_text) :
        SearchServer(SplitIntoWords(stop_words_text)) {
}


int SearchServer::GetDocumentCount() const {
    return document_count_;
}

vector<string> SearchServer::SplitIntoWords(const string &text) const {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                if (!IsValidString(word)) {
                    throw invalid_argument(
                            "Слово `"s + word
                                    + "` имеет запрещенные символы."s);
                }
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }

    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

void SearchServer::AddDocument(int document_id, const string &document,
        DocumentStatus status, const vector<int> &rating) {

    PossibleAddDocument(document_id, document);

    const vector<string> words = SplitIntoWordsNoStop(document);
    int count_words = words.size();
    double frequency_occurrence_word = 1. / count_words;
    for (const string &word : words) {
        word_to_document_freqs_[word][document_id] += frequency_occurrence_word;
    }
    properties_documents_[document_id] =
            { ComputeAverageRating(rating), status };
    insert_doc_.push_back(document_id);
    ++document_count_;
}

vector<Document> SearchServer::FindTopDocuments(const string &raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(const string &raw_query,
        DocumentStatus find_status) const {
    return FindTopDocuments(raw_query,
            [&find_status](int document_id, DocumentStatus status, int rating) {
                return status == find_status;
            });
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(
        const string &raw_query, int document_id) const {
    Query query;

    ParseQuery(raw_query, query);
    CheckQurey(query);

    vector<string> v_result;
    DocumentStatus doc_stat = DocumentStatus::ACTUAL;

    auto interator = properties_documents_.find(document_id);
    if (interator != properties_documents_.end()) {
        doc_stat = interator->second.status; // @suppress("Field cannot be resolved")
    }

    if (query.minus_words.size() != 0) {
        for (const string &minus_word : query.minus_words) {
            auto map_word = word_to_document_freqs_.find(minus_word);
            if (map_word != word_to_document_freqs_.end()) {
                auto map_doc_id = map_word->second.find(document_id);
                if (map_doc_id != map_word->second.end()) {
                    return tuple(v_result, doc_stat);
                }
            }
        }
    }

    if (query.plus_words.size() != 0) {
        for (const string &plus_word : query.plus_words) {
            auto map_word = word_to_document_freqs_.find(plus_word);
            if (map_word != word_to_document_freqs_.end()) {
                auto map_doc_id = map_word->second.find(document_id);
                if (map_doc_id != map_word->second.end()) {
                    v_result.push_back(plus_word);
                }
            }
        }
    }
    sort(v_result.begin(), v_result.end());
    return tuple(v_result, doc_stat);
}

int SearchServer::GetDocumentId(int index) const {

    if (index < 0 || index >= static_cast<int>(insert_doc_.size())) {
        throw out_of_range(
                "Значение индекса документа выходит за пределы допустимого диапазона."s);
    }

    return insert_doc_.at(index);
}

SearchServer::DocumentProperties SearchServer::GetPropertiesDocument(
        const int &id) const {
    DocumentProperties doc_result;
    const auto &interator = properties_documents_.find(id);
    if (interator != properties_documents_.end()) {
        return interator->second;
    }
    return doc_result;
}

int SearchServer::ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int sum = accumulate(ratings.begin(), ratings.end(), 0);
    int rating = ratings.size();
    return sum / rating;
}

bool SearchServer::IsStopWord(const string &word) const {
    if (stop_words_.size() != 0) {
        return stop_words_.count(word) > 0;
    }
    return false;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string &text) const {
    vector<string> words;

    for (const string &word : SplitIntoWords(text)) {
        if (!IsValidString(word)) {
            throw invalid_argument(
                    "Текст `"s + text + "` содержит запрещенные символы."s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

bool SearchServer::IsValidString(const string &str) {
    return none_of(str.begin(), str.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

void SearchServer::PossibleAddDocument(int document_id,
        const string &document) const {
    if (document_id < 0) // id документа не может быть меньше нуля
        throw invalid_argument(
                "Идентификатор документа `"s + document + "` меньше нуля."s);
    auto result = find(insert_doc_.begin(), insert_doc_.end(), document_id);
    if (result != end(insert_doc_)) { // проверка на добавленные идентификаторы документов
        throw invalid_argument(
                "Идентификатор документа `"s + to_string(document_id)
                        + "` уже был добавлен."s);
    }
    if (document.empty())
        throw invalid_argument(
                "Документ с идентификатором `"s + to_string(document_id)
                        + "` пустой."s);  // Документ не может быть пустой
}

void SearchServer::ParseQuery(const string &text, Query &query) const {
    if (!text.empty()) {
        for (const string &word : SplitIntoWordsNoStop(text)) {
            if (word[0] != '-')
                query.plus_words.insert(word);
            else {
                query.minus_words.push_back(word.substr(1));
            }
        }
    }
}

void SearchServer::CheckQurey(Query &query) const {
    for (const string &word : query.minus_words) {
        if (word[0] == '-') {
            throw invalid_argument("Запрос содержит слово с двумя знаками -"s);
        }
        if (word.size() == 0l) {
            throw invalid_argument("Запрос содержит пустые слова."s);
        }
    }
}

double SearchServer::CalcIDF(const string &word) const {
    return log(
            static_cast<double>(document_count_)
                    / static_cast<double>(word_to_document_freqs_.at(word).size()));
}


//============================================================================
// Name        : YaPrakticum_SearchEngine.cpp
// Author      : Vitaly Sandalov
// Version     : 0.1
// Copyright   : -
// Description : Search engine
//============================================================================

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
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

struct Document {
    int id;
    int relevance;
};


class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words){
        	set<int>& set_tmp = documents_[word];
        	set_tmp.insert(document_id);
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        set<string> query_words;
        Query query;
        ParseQuery(raw_query, query);
        auto matched_documents = FindAllDocuments(query);

        auto by_relevance = [](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance;
        };

        sort(matched_documents.begin(), matched_documents.end(), by_relevance);
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

      struct Query {
          set<string> plus_words;
          set<string> minus_words;
      };

    map<string, set<int>> documents_;

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    void ParseQuery(const string& text,
    		Query& query) const {
        for (const string& word : SplitIntoWordsNoStop(text)) {
        	if (word[0] != '-')
        		query.plus_words.insert(word);
        	else {
        		query.minus_words.insert(word.substr(1));
        	}
        }
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> matched_documents;
        map<int,int> query_result;
        for (const string& plus_word : query.plus_words) {
        	const set<int>& tempset = documents_.find(plus_word)->second;
        	for (int id : tempset){
        		int& rel = query_result[id];
        		++rel;
        	 }
        }
        for (const string& minus_word : query.minus_words) {
        	const set<int>& tempset = documents_.find(minus_word)->second;
        	for (int id : tempset){
        		query_result.erase(id);
       	 }
        }

        for (auto& res : query_result){
        	matched_documents.push_back({res.first, res.second});
       }

        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}

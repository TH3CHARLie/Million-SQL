#include <API/API.h>
#include <Interpreter/AST.h>
#include <SQLError.h>
#include <iostream>
#include <stdexcept>

namespace Interpreter {
namespace AST {

void CreateTableStatement::setTableName(const std::string &name) {
    tableName = name;
}

void CreateTableStatement::addAttribute(const Attribute &attr) {
    if (attributes.size() < 32) {
        attributes.push_back(attr);
    } else {
        throw SQLError("tables with more than 32 attributes are not supported");
    }
}

void CreateTableStatement::addPrimaryKey(const std::string &id) {
    if (primaryKey.empty()) {
        primaryKey = id;
    } else {
        throw SQLError("multiple primary keys are declared");
    }
}

void DropTableStatement::setTableName(const std::string &name) {
    tableName = name;
}

void CreateIndexStatement::setIndexName(const std::string &name) {
    indexName = name;
}

void CreateIndexStatement::setTableName(const std::string &name) {
    tableName = name;
}

void CreateIndexStatement::setAttrName(const std::string &name) {
    attrName = name;
}

void DropIndexStatement::setTableName(const std::string &name) {
    indexName = name;
}

void SelectStatement::setTableName(const std::string &name) {
    tableName = name;
}

void SelectStatement::addAttrName(const std::string &name) {
    attributes.push_back(name);
}

void SelectStatement::addPredicate(const Predicate &predicate) {
    predicates.push_back(predicate);
}

void InsertStatement::setTableName(const std::string &name) {
    tableName = name;
}

void InsertStatement::addValue(const Value &value) { values.push_back(value); }

void DeleteStatement::setTableName(const std::string &name) {
    tableName = name;
}

void DeleteStatement::addPredicate(const Predicate &predicate) {
    predicates.push_back(predicate);
}

void ExecfileStatement::setFilePath(const std::string &path) {
    filePath = path;
}

const std::string &ExecfileStatement::getFilePath() const { return filePath; }

void CreateTableStatement::callAPI() const {
    if (primaryKey.empty()) {
        throw SQLError("primary key not specified");
    }
    API::createTable(tableName, primaryKey, attributes);
}

void DropTableStatement::callAPI() const { API::dropTable(tableName); }

void CreateIndexStatement::callAPI() const {
    API::createIndex(indexName, tableName, attrName);
}

void DropIndexStatement::callAPI() const { API::dropIndex(indexName); }

void SelectStatement::callAPI() const {
    auto records = API::select(attributes, tableName, predicates);
    for (auto record : records) {
        int size = record.size();
        if (size > 1) {
            std::cout << "(";
        }
        for (int i = 0; i < size; ++i) {
            std::cout << record[i].toString();
            if (i != size - 1) {
                std::cout << ", ";
            }
        }
        if (size > 1) {
            std::cout << ")";
        }
        std::cout << std::endl;
    }
}

void InsertStatement::callAPI() const { API::insert(tableName, values); }

void DeleteStatement::callAPI() const {
    API::deleteFrom(tableName, predicates);
}

void QuitStatement::callAPI() const {
    throw std::logic_error("no API for 'quit'");
}

void ExecfileStatement::callAPI() const {
    throw std::logic_error("no API for 'execfile'");
}

} // namespace AST
} // namespace Interpreter
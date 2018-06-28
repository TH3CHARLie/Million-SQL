#include <BufferManager/Block.h>
#include <CatalogManager/CatalogManager.h>
#include <CatalogManager/TableSpec.h>
#include <algorithm>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace CM {

std::list<std::shared_ptr<Schema>> schemas;
std::unordered_map<std::string, std::shared_ptr<Schema>> mapSchemas;
std::unordered_map<std::string, uint32_t> mapSchemaOffsets;

bool hasTable(const std::string &tableName) {
    return mapSchemas.find(tableName) != mapSchemas.end();
}

void init() {
    schemas.clear();
    mapSchemas.clear();
    mapSchemaOffsets.clear();

    auto filename = File::catalogFilename();
    if (!BM::fileExists(filename)) {
        BM::createFile(filename, File::FileType::CATALOG);
    }
    BM::PtrBlock blk0 = BM::readBlock(BM::makeID(filename, 0));
    blk0->resetPos();
    File::catalogFileHeader header;
    blk0->read(reinterpret_cast<char *>(&header), sizeof(header));

    uint32_t currP = header.tableOffset, nextP = 0;
    static char strbuf[NAME_LENGTH];
    while (currP != 0) {
        BM::PtrBlock blk = BM::readBlock(BM::makeID(filename, currP));
        blk->resetPos();
        auto schema = std::make_shared<Schema>();
        uint32_t numAttrs = 0;
        blk->read(reinterpret_cast<char *>(&nextP), sizeof(uint32_t));
        if (nextP & DELETED_MARK) {
            currP = nextP & DELETED_MASK;
            continue;
        }
        blk->read(reinterpret_cast<char *>(&numAttrs), sizeof(uint32_t));
        blk->read(strbuf, NAME_LENGTH);
        schema->tableName = std::string(strbuf);
        blk->read(strbuf, NAME_LENGTH);
        schema->primaryKey = std::string(strbuf);
        for (auto i = 0u; i != numAttrs; ++i) {
            Attribute attribute;
            blk->read(strbuf, NAME_LENGTH);
            attribute.name = std::string(strbuf);
            uint32_t bin;
            blk->read(reinterpret_cast<char *>(&bin), sizeof(uint32_t));
            std::tie(attribute.type, attribute.charCnt, attribute.isUnique) =
                decodeProperties(bin);
            schema->attributes.push_back(attribute);
        }
        schemas.push_back(schema);
        mapSchemas[schema->tableName] = schema;
        mapSchemaOffsets[schema->tableName] = currP;
        currP = nextP;
    }
}

void createTable(const std::string &tableName, const std::string &primaryKey,
                 const std::vector<Attribute> &attributes) {
    if (hasTable(tableName)) {
        throw SQLError("table \'" + tableName + "\' already exists");
    }
    auto schema = std::make_shared<Schema>();
    schema->tableName = tableName;
    schema->primaryKey = primaryKey;
    schema->attributes = attributes;
    schemas.push_front(schema);
    mapSchemas[tableName] = schema;

    auto filename = File::catalogFilename();
    BM::PtrBlock blk0 = BM::readBlock(BM::makeID(filename, 0));
    File::catalogFileHeader header;
    blk0->resetPos();
    blk0->read(reinterpret_cast<char *>(&header), sizeof(header));

    uint32_t newP = header.blockNum++;
    uint32_t nextP = header.tableOffset;
    uint32_t numAttrs = attributes.size();
    header.tableOffset = newP;
    mapSchemaOffsets[tableName] = newP;

    uint32_t offset0 = 0, offsetP = 0;
    auto write0 = [&](const char *src, size_t size) {
        BM::writeBlock(BM::makeID(filename, 0), src, offset0, size);
        offset0 += size;
    };
    auto writeP = [&](const char *src, size_t size) {
        BM::writeBlock(BM::makeID(filename, newP), src, offsetP, size);
        offsetP += size;
    };
    write0(reinterpret_cast<const char *>(&header), sizeof(header));

    writeP(reinterpret_cast<const char *>(&nextP), sizeof(uint32_t));
    writeP(reinterpret_cast<const char *>(&numAttrs), sizeof(uint32_t));
    static char strbuf[NAME_LENGTH];

    memset(strbuf, 0, NAME_LENGTH);
    std::strcpy(strbuf, tableName.c_str());
    writeP(strbuf, NAME_LENGTH);

    memset(strbuf, 0, NAME_LENGTH);
    std::strcpy(strbuf, primaryKey.c_str());
    writeP(strbuf, NAME_LENGTH);

    for (auto &attribute : attributes) {
        uint32_t bin = encodeProperties(attribute);
        memset(strbuf, 0, NAME_LENGTH);
        std::strcpy(strbuf, attribute.name.c_str());
        writeP(strbuf, NAME_LENGTH);
        writeP(reinterpret_cast<const char *>(&bin), sizeof(uint32_t));
    }
}

void dropTable(const std::string &tableName) {
    if (!hasTable(tableName)) {
        throw SQLError("table \'" + tableName + "\' does not exist");
    }
    auto filename = File::catalogFilename();
    auto schema = mapSchemas[tableName];
    uint32_t offset = mapSchemaOffsets[tableName], nextP;

    BM::PtrBlock blk = BM::readBlock(BM::makeID(filename, offset));
    blk->resetPos(0);
    blk->read(reinterpret_cast<char *>(&nextP), sizeof(uint32_t));
    nextP |= DELETED_MARK;
    BM::writeBlock(BM::makeID(filename, offset),
                   reinterpret_cast<const char *>(&nextP), 0, sizeof(uint32_t));
    schemas.erase(std::find(schemas.begin(), schemas.end(), schema));
    mapSchemas.erase(tableName);
    mapSchemaOffsets.erase(tableName);
}

void createIndex(const std::string &indexName, const std::string &tableName,
                 const std::string &attrName) {
    //
}

void dropIndex(const std::string &indexName) {
    //
}

void exit() {}

std::shared_ptr<Schema> getSchema(const std::string &tableName) {
    auto i = mapSchemas.find(tableName);
    if (i != mapSchemas.end()) {
        return i->second;
    } else {
        throw SQLError("cannot find table \'" + tableName + "\'");
    }
}

} // namespace CM
#include "notestore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <QDebug>

// --- NoteMetadata ---

QJsonObject NoteMetadata::toJson() const {
    return {
        {"id", id},
        {"title", title},
        {"fileName", fileName},
        {"category", category},
        {"createdAt", createdAt.toString(Qt::ISODate)},
        {"updatedAt", updatedAt.toString(Qt::ISODate)},
        {"wordCount", wordCount},
        {"preview", preview},
    };
}

NoteMetadata NoteMetadata::fromJson(const QJsonObject &obj) {
    NoteMetadata m;
    m.id        = obj.value("id").toString();
    m.title     = obj.value("title").toString();
    m.fileName  = obj.value("fileName").toString();
    m.category  = obj.value("category").toString();
    m.createdAt = QDateTime::fromString(obj.value("createdAt").toString(), Qt::ISODate);
    m.updatedAt = QDateTime::fromString(obj.value("updatedAt").toString(), Qt::ISODate);
    m.wordCount = obj.value("wordCount").toInt();
    m.preview   = obj.value("preview").toString();
    return m;
}

// --- NoteStore ---

NoteStore::NoteStore(const QString &baseDir)
    : m_baseDir(baseDir)
    , m_notesDir(baseDir + "/notes")
{
    QDir().mkpath(m_notesDir);
}

// --- 路径工具 ---

QString NoteStore::metadataPath() const {
    return m_baseDir + "/metadata.json";
}

QString NoteStore::notePath(const QString &fileName, const QString &category) const {
    if (category.isEmpty())
        return m_notesDir + "/" + fileName;
    return m_notesDir + "/" + category + "/" + fileName;
}

QString NoteStore::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// --- 元数据读写 ---

QVector<NoteMetadata> NoteStore::loadMetadata() const {
    QFile f(metadataPath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return {};
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    QVector<NoteMetadata> list;
    for (const auto &val : doc.object().value("notes").toArray()) {
        list.append(NoteMetadata::fromJson(val.toObject()));
    }
    return list;
}

void NoteStore::saveMetadata(const QVector<NoteMetadata> &list) const {
    QJsonArray arr;
    for (const auto &m : list)
        arr.append(m.toJson());
    QJsonObject root;
    root["notes"] = arr;

    QFile f(metadataPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(root);
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
    }
}

// --- 工具 ---

NoteMetadata NoteStore::metadataFromNote(const Note &note) const {
    NoteMetadata m;
    m.id        = note.id;
    m.title     = note.title;
    m.fileName  = note.fileName;
    m.category  = note.category;
    m.createdAt = note.createdAt;
    m.updatedAt = note.updatedAt;
    m.wordCount = note.wordCount;
    m.preview   = makePreview(note.content);
    return m;
}

int NoteStore::countWords(const QString &text) const {
    // 简单按空白字符计数
    return text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
}

QString NoteStore::makePreview(const QString &text, int maxLen) const {
    QString plain = text.left(maxLen).replace('\n', ' ');
    if (text.length() > maxLen) plain += "…";
    return plain;
}

// --- 笔记 CRUD ---

QVector<NoteMetadata> NoteStore::listNotes() const {
    auto metadata = loadMetadata();
    // 过滤掉文件已被删除的条目
    metadata.erase(std::remove_if(metadata.begin(), metadata.end(), [this](const NoteMetadata &m) {
        return !QFile::exists(notePath(m.fileName, m.category));
    }), metadata.end());
    // 按更新时间倒序
    std::sort(metadata.begin(), metadata.end(), [](const NoteMetadata &a, const NoteMetadata &b) {
        return a.updatedAt > b.updatedAt;
    });
    return metadata;
}

Note NoteStore::readNote(const QString &id) const {
    auto metadata = loadMetadata();
    for (const auto &m : metadata) {
        if (m.id == id) {
            QString path = notePath(m.fileName, m.category);
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly))
                throw std::runtime_error("Cannot read note file");
            Note note;
            note.id        = m.id;
            note.title     = m.title;
            note.fileName  = m.fileName;
            note.category  = m.category;
            note.createdAt = m.createdAt;
            note.updatedAt = m.updatedAt;
            note.wordCount = m.wordCount;
            note.content   = QString::fromUtf8(f.readAll());
            f.close();
            return note;
        }
    }
    throw std::runtime_error("Note not found: " + id.toStdString());
}

Note NoteStore::createNote(const SaveNoteRequest &req) {
    QString id = generateId();
    QString fileName = id + ".md";
    QDateTime now = QDateTime::currentDateTime();

    // 分类目录
    if (!req.category.isEmpty())
        QDir().mkpath(m_notesDir + "/" + req.category);

    QString path = notePath(fileName, req.category);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        throw std::runtime_error("Cannot create note file");
    f.write(req.content.toUtf8());
    f.close();

    Note note;
    note.id        = id;
    note.title     = req.title;
    note.fileName  = fileName;
    note.category  = req.category;
    note.createdAt = now;
    note.updatedAt = now;
    note.wordCount = countWords(req.content);
    note.content   = req.content;

    auto metadata = loadMetadata();
    metadata.append(metadataFromNote(note));
    saveMetadata(metadata);
    return note;
}

Note NoteStore::updateNote(const QString &id, const SaveNoteRequest &req) {
    auto metadata = loadMetadata();
    for (auto &m : metadata) {
        if (m.id == id) {
            QString path = notePath(m.fileName, m.category);
            QFile f(path);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                throw std::runtime_error("Cannot write note file");
            f.write(req.content.toUtf8());
            f.close();

            m.title     = req.title;
            m.updatedAt = QDateTime::currentDateTime();
            m.wordCount = countWords(req.content);
            m.preview   = makePreview(req.content);

            if (m.category != req.category) {
                // 移动文件
                QString oldPath = notePath(m.fileName, m.category);
                QString newDir  = m_notesDir + "/" + req.category;
                QDir().mkpath(newDir);
                QString newPath = notePath(m.fileName, req.category);
                QFile::rename(oldPath, newPath);
                m.category = req.category;
            }

            saveMetadata(metadata);

            Note note;
            note.id        = m.id;
            note.title     = m.title;
            note.fileName  = m.fileName;
            note.category  = m.category;
            note.createdAt = m.createdAt;
            note.updatedAt = m.updatedAt;
            note.wordCount = m.wordCount;
            note.content   = req.content;
            return note;
        }
    }
    throw std::runtime_error("Note not found: " + id.toStdString());
}

void NoteStore::deleteNote(const QString &id) {
    auto metadata = loadMetadata();
    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        if (it->id == id) {
            QString path = notePath(it->fileName, it->category);
            QFile::remove(path);
            metadata.erase(it);
            saveMetadata(metadata);
            return;
        }
    }
}

// --- 分类 ---

QStringList NoteStore::listCategories() const {
    auto metadata = loadMetadata();
    QSet<QString> cats;
    for (const auto &m : metadata) {
        if (!m.category.isEmpty())
            cats.insert(m.category);
    }
    QStringList list(cats.begin(), cats.end());
    list.sort();
    return list;
}

void NoteStore::createCategory(const QString &name) {
    QDir().mkpath(m_notesDir + "/" + name);
}

void NoteStore::renameCategory(const QString &oldName, const QString &newName) {
    QDir().mkpath(m_notesDir + "/" + newName);
    auto metadata = loadMetadata();
    for (auto &m : metadata) {
        if (m.category == oldName) {
            QString oldPath = notePath(m.fileName, oldName);
            QString newPath = notePath(m.fileName, newName);
            QFile::rename(oldPath, newPath);
            m.category = newName;
        }
    }
    saveMetadata(metadata);
    QDir().rmdir(m_notesDir + "/" + oldName);
}

void NoteStore::deleteCategory(const QString &name) {
    auto metadata = loadMetadata();
    for (auto &m : metadata) {
        if (m.category == name) {
            QString path = notePath(m.fileName, name);
            QFile::remove(path);
            m.category.clear();
        }
    }
    saveMetadata(metadata);
    QDir(m_notesDir + "/" + name).removeRecursively();
}

NoteMetadata NoteStore::moveNoteToCategory(const QString &id, const QString &category) {
    auto metadata = loadMetadata();
    for (auto &m : metadata) {
        if (m.id == id) {
            QString oldPath = notePath(m.fileName, m.category);
            if (!category.isEmpty())
                QDir().mkpath(m_notesDir + "/" + category);
            QString newPath = notePath(m.fileName, category);
            if (oldPath != newPath) {
                QFile::rename(oldPath, newPath);
            }
            m.category = category;
            saveMetadata(metadata);
            return m;
        }
    }
    throw std::runtime_error("Note not found");
}

// --- 导入导出 ---

Note NoteStore::importMarkdownFile(const QString &filePath, const QString &category) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error("Cannot open file for import");

    QString content = QString::fromUtf8(f.readAll());
    f.close();

    QFileInfo fi(filePath);
    QString title = fi.completeBaseName();

    SaveNoteRequest req;
    req.title    = title;
    req.content  = content;
    req.category = category;
    return createNote(req);
}

void NoteStore::exportMarkdownFile(const QString &id, const QString &filePath) const {
    auto note = const_cast<NoteStore*>(this)->readNote(id);
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        throw std::runtime_error("Cannot open file for export");
    f.write(note.content.toUtf8());
    f.close();
}

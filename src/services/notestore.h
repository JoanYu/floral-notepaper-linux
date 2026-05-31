// NoteStore — 笔记存储服务 (对应原 services/notes.rs)
#ifndef NOTESTORE_H
#define NOTESTORE_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>
#include <QVector>

struct NoteMetadata {
    QString id;
    QString title;
    QString fileName;
    QString category;
    QDateTime createdAt;
    QDateTime updatedAt;
    int wordCount = 0;
    QString preview;

    QJsonObject toJson() const;
    static NoteMetadata fromJson(const QJsonObject &obj);
};

struct Note {
    QString id;
    QString title;
    QString fileName;
    QString category;
    QDateTime createdAt;
    QDateTime updatedAt;
    int wordCount = 0;
    QString content;
};

struct SaveNoteRequest {
    QString title;
    QString content;
    QString category;
};

class NoteStore {
public:
    explicit NoteStore(const QString &baseDir);

    // 笔记 CRUD
    QVector<NoteMetadata> listNotes() const;
    Note readNote(const QString &id) const;
    Note createNote(const SaveNoteRequest &req);
    Note updateNote(const QString &id, const SaveNoteRequest &req);
    void deleteNote(const QString &id);

    // 分类
    QStringList listCategories() const;
    void createCategory(const QString &name);
    void renameCategory(const QString &oldName, const QString &newName);
    void deleteCategory(const QString &name);
    NoteMetadata moveNoteToCategory(const QString &id, const QString &category);

    // 导入导出
    Note importMarkdownFile(const QString &filePath, const QString &category);
    void exportMarkdownFile(const QString &id, const QString &filePath) const;

private:
    QString m_baseDir;
    QString m_notesDir;

    QString metadataPath() const;
    QString notePath(const QString &fileName, const QString &category) const;
    QString generateId() const;
    QVector<NoteMetadata> loadMetadata() const;
    void saveMetadata(const QVector<NoteMetadata> &list) const;
    NoteMetadata metadataFromNote(const Note &note) const;
    int countWords(const QString &text) const;
    QString makePreview(const QString &text, int maxLen = 80) const;
};

#endif // NOTESTORE_H

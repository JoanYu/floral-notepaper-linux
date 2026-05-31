// MarkdownPreview — Markdown 预览 via QTextBrowser + md4c
#ifndef MARKDOWNPREVIEW_H
#define MARKDOWNPREVIEW_H

#include <QTextBrowser>

class MarkdownPreview : public QTextBrowser {
    Q_OBJECT
public:
    explicit MarkdownPreview(QWidget *parent = nullptr);
    void setMarkdownContent(const QString &markdown, int fontSize = 14, const QString &markdownTheme = "github");

private:
    QString buildHtml(const QString &markdown, int fontSize, const QString &markdownTheme) const;
    QString m_lastMarkdown;
    int m_lastFontSize = 14;
};

#endif // MARKDOWNPREVIEW_H

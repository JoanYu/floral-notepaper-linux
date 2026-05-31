#include "markdownpreview.h"
#include "app/theme.h"

extern "C" {
#include "3rdparty/md4c/md4c-html.h"
}

MarkdownPreview::MarkdownPreview(QWidget *parent)
    : QTextBrowser(parent)
{
    setContextMenuPolicy(Qt::NoContextMenu);
    setOpenExternalLinks(false);
    setOpenLinks(false);
    setReadOnly(true);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    document()->setDocumentMargin(0);
    setViewportMargins(12, 8, 12, 8);
    setStyleSheet(R"(
        QTextBrowser { background: transparent; border: none; }
        QTextBrowser QScrollBar:vertical {
            width: 8px;
            background: transparent;
            margin: 0;
            border: none;
        }
        QTextBrowser QScrollBar::handle:vertical {
            background-color: #b8b8ae;
            border-radius: 4px;
            min-height: 40px;
        }
        QTextBrowser QScrollBar::handle:vertical:hover { background-color: #8a8a80; }
        QTextBrowser QScrollBar::add-line:vertical,
        QTextBrowser QScrollBar::sub-line:vertical { height: 0; border: none; background: transparent; }
        QTextBrowser QScrollBar::add-page:vertical,
        QTextBrowser QScrollBar::sub-page:vertical { background: transparent; border: none; }
        QTextBrowser QScrollBar:horizontal { height: 0; background: transparent; border: none; }
    )");
}

void MarkdownPreview::setMarkdownContent(const QString &markdown, int fontSize, const QString &markdownTheme) {
    m_lastMarkdown = markdown;
    m_lastFontSize = fontSize;
    setHtml(buildHtml(markdown, fontSize, markdownTheme));
}

static void processTables(QString &html, const QString &borderColor, const QString &altBg) {
    // Find the first <table> and last </table> to only wrap the table itself
    int tableStart = html.indexOf("<table>");
    int tableEnd = html.lastIndexOf("</table>");
    if (tableStart == -1 || tableEnd == -1 || tableEnd <= tableStart) return;

    QString tableContent = html.mid(tableStart, tableEnd - tableStart + strlen("</table>"));
    QString wrapper = QString("<div style=\"border:1px solid %1;border-radius:4px;overflow:hidden;margin:0.8em 0;\">"
                             "<table style=\"border-collapse:collapse;width:100%;font-size:0.875rem;\">").arg(borderColor);

    // Style <th>: bold, 4-sided border, top/bottom padding for vertical centering
    tableContent.replace("<th>",
        QString("<th style=\"font-weight:bold;border:1px solid %1;"
        "padding:8px 13px;text-align:left;line-height:1.5;background:%2;\">").arg(borderColor).arg(altBg));

    // Style <td>: 4-sided border, top/bottom padding
    tableContent.replace("<td>",
        QString("<td style=\"border:1px solid %1;"
        "padding:8px 13px;text-align:left;line-height:1.5;\">").arg(borderColor));
    tableContent.replace("</td>", "</td>");

    // Remove thead/tbody wrappers that Qt rich text may mishandle
    tableContent.replace("<thead>", "").replace("</thead>", "");
    tableContent.replace("<tbody>", "").replace("</tbody>", "");

    // Reconstruct: everything before table + wrapper + tableContent + closing div + everything after table
    QString before = html.mid(0, tableStart);
    QString after = html.mid(tableEnd + strlen("</table>"));
    html = before + wrapper + tableContent + "</table></div>" + after;
}

// Inject data-line attributes into HTML for block-based scroll sync
static void injectDataLineAttrs(QString &html, const QString &markdown) {
    // Count markdown blocks
    QStringList blocks = markdown.split("\n\n", Qt::SkipEmptyParts);
    QVector<int> blockLines;
    int pos = 0;
    for (const QString &block : blocks) {
        blockLines.append(pos);
        pos += block.size() + 2; // +2 for \n\n
    }

    // Wrap each block-level element with a data-line attribute
    // We'll use simple string replacement for block tags
    // Block tags to process (in order they appear in HTML)
    QStringList blockTags = {"<p", "<h1", "<h2", "<h3", "<h4", "<h5", "<h6",
                              "<pre", "<ul", "<ol", "<blockquote", "<table",
                              "<li", "<hr"};

    int currentBlock = 0;
    int blockCount = blocks.size();

    // Process each block
    for (int i = 0; i < blockLines.size() && currentBlock < blockCount; ++i) {
        int lineNum = i + 1; // 1-indexed
        // We need to find where this block's HTML appears in the output
        // and inject data-line into the opening tag
        // This is a best-effort approach based on order

        // Find the corresponding HTML element in order
        // Since md4c outputs in order, we process sequentially
    }

    // Simpler approach: walk through HTML and assign data-line by counting block tags
    int lineIdx = 1;
    for (const QString &block : blocks) {
        QString marker = QString(" data-line=\"%1\"").arg(lineIdx);
        // Find next unprocessed block tag and add attribute
        for (const QString &tag : blockTags) {
            QString search = tag;
            int tagPos = html.indexOf(search);
            if (tagPos != -1) {
                int gtPos = html.indexOf(">", tagPos);
                if (gtPos != -1 && gtPos - tagPos < 50) { // sanity check
                    QString openTag = html.mid(tagPos, gtPos - tagPos + 1);
                    if (!openTag.contains("data-line")) {
                        QString newTag = openTag;
                        // Insert before the closing >
                        newTag.insert(newTag.length() - 1, marker);
                        html.replace(openTag, newTag, Qt::CaseSensitive);
                    }
                }
                break;
            }
        }
        ++lineIdx;
    }
}

static QString md4cToHtml(const QString &markdown,
                           const QString &borderColor,
                           const QString &altBg) {
    QByteArray input = markdown.toUtf8();
    QByteArray output;
    md_html(input.constData(), input.size(),
            [](const MD_CHAR *html, MD_SIZE size, void *userdata) {
                static_cast<QByteArray *>(userdata)->append(html, size);
            }, &output,
            MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS,
            0);
    QString html = QString::fromUtf8(output);
    processTables(html, borderColor, altBg);
    return html;
}

QString MarkdownPreview::buildHtml(const QString &markdown, int fontSize, const QString &markdownTheme) const {
    auto& p = FloralTheme::instance().current();
    bool dark = p.paper.lightness() < 128;
    bool useClaude = (markdownTheme == "claude");

    // ── GitHub Light colors ──
    QString gh_bg        = "#ffffff";
    QString gh_bg_soft   = "#f8f8f8";
    QString gh_border    = "#ddd";
    QString gh_text      = "#333333";
    QString gh_text_soft = "#777777";
    QString gh_link     = "#4183C4";
    QString gh_code_bg  = "#f8f8f8";
    QString gh_code_txt = "#333333";
    QString gh_pre_bg   = "#f8f8f8";
    QString gh_pre_txt  = "#333333";
    QString gh_h1_line  = "#eee";
    QString gh_hr_bg    = "#e7e7e7";

    // ── GitHub Night colors ──
    QString ghn_bg       = "#0d1117";
    QString ghn_bg_soft  = "#161b22";
    QString ghn_border   = "#21262d";
    QString ghn_text     = "#c9d1d9";
    QString ghn_text_soft= "#8b949e";
    QString ghn_link     = "#58a6ff";
    QString ghn_code_bg  = "#161b22";
    QString ghn_code_txt = "#c9d1d9";
    QString ghn_pre_bg   = "#161b22";
    QString ghn_pre_txt  = "#c9d1d9";
    QString ghn_h1_line = "#21262d";
    QString ghn_hr_bg   = "#21262d";

    // ── Claude Light colors ──
    QString cl_bg       = "#faf9f5";
    QString cl_border   = "#1f1e1d26";
    QString cl_text     = "#141413";
    QString cl_text_soft= "#3d3d3a";
    QString cl_link    = "#141413";
    QString cl_code_bg  = "#3d3d3a0d";
    QString cl_code_txt = "#8a2424";
    QString cl_pre_bg   = "#ffffff80";
    QString cl_pre_txt  = "#141413";
    QString cl_hr_bg   = "#1f1e1d4d";
    QString cl_quote_bg = "#1f1e1d1a";
    QString cl_table_th = "#1f1e1d99";
    QString cl_table_td = "#1f1e1d4d";

    // ── Claude Dark colors ──
    QString cld_bg       = "#262624";
    QString cld_border   = "#dedcd126";
    QString cld_text     = "#faf9f5";
    QString cld_text_soft= "#c2c0b6";
    QString cld_link    = "#faf9f5";
    QString cld_code_bg  = "#c2c0b60d";
    QString cld_code_txt = "#fe8181";
    QString cld_pre_bg   = "#30302E80";
    QString cld_pre_txt  = "#faf9f5";
    QString cld_hr_bg    = "#dedcd14d";
    QString cld_quote_bg = "#dedcd126";
    QString cld_table_th = "#dedcd199";
    QString cld_table_td = "#dedcd14d";

    QString bg, text, text_soft, link, code_bg, code_txt, pre_bg, pre_txt;
    QString h1_line, hr_bg, quote_border, quote_text, table_th, table_td, quote_bg;
    QString blockquote_bg, strong_color;

    if (useClaude) {
        if (dark) {
            bg          = cld_bg;
            text        = cld_text;
            text_soft   = cld_text_soft;
            link        = cld_link;
            code_bg     = cld_code_bg;
            code_txt    = cld_code_txt;
            pre_bg      = cld_pre_bg;
            pre_txt     = cld_pre_txt;
            h1_line     = cld_border;
            hr_bg       = cld_hr_bg;
            quote_border= cld_quote_bg;
            quote_text  = cld_text_soft;
            table_th    = cld_table_th;
            table_td    = cld_table_td;
            blockquote_bg = "transparent";
            strong_color= cld_text;
            quote_bg    = "#1c2e22";
        } else {
            bg          = cl_bg;
            text        = cl_text;
            text_soft   = cl_text_soft;
            link        = cl_link;
            code_bg     = cl_code_bg;
            code_txt    = cl_code_txt;
            pre_bg      = cl_pre_bg;
            pre_txt     = cl_pre_txt;
            h1_line     = cl_border;
            hr_bg       = cl_hr_bg;
            quote_border= cl_quote_bg;
            quote_text  = cl_text_soft;
            table_th    = cl_table_th;
            table_td    = cl_table_td;
            blockquote_bg = "transparent";
            strong_color= cl_text;
            quote_bg    = "#e8f0eb";
        }
    } else {
        // GitHub theme
        if (dark) {
            bg          = ghn_bg;
            text        = ghn_text;
            text_soft   = ghn_text_soft;
            link        = ghn_link;
            code_bg     = ghn_code_bg;
            code_txt    = ghn_code_txt;
            pre_bg      = ghn_pre_bg;
            pre_txt     = ghn_pre_txt;
            h1_line     = ghn_h1_line;
            hr_bg       = ghn_hr_bg;
            quote_border= ghn_border;
            quote_text  = ghn_text_soft;
            table_th    = ghn_border;
            table_td    = ghn_border;
            blockquote_bg = "transparent";
            strong_color= ghn_text;
            quote_bg    = ghn_bg_soft;
        } else {
            bg          = gh_bg;
            text        = gh_text;
            text_soft   = gh_text_soft;
            link        = gh_link;
            code_bg     = gh_code_bg;
            code_txt    = gh_code_txt;
            pre_bg      = gh_pre_bg;
            pre_txt     = gh_pre_txt;
            h1_line     = gh_h1_line;
            hr_bg       = gh_hr_bg;
            quote_border= gh_border;
            quote_text  = gh_text_soft;
            table_th    = gh_border;
            table_td    = gh_border;
            blockquote_bg = "transparent";
            strong_color= gh_text;
            quote_bg    = gh_bg_soft;
        }
    }

    QString css = R"(
        body {
            font-family: "Noto Sans SC", "Source Han Sans SC", system-ui, sans-serif;
            font-size: )" + QString::number(fontSize) + R"(px;
            color: )" + text_soft + R"(;
            background: transparent;
            line-height: 1.9;
            margin: 0;
            max-width: 560px;
        }
        .md-content {
            padding: 0;
        }

        h1, h2, h3, h4 {
            font-family: "Noto Serif SC", "Source Han Serif SC", Georgia, serif;
            color: )" + text + R"(;
            font-weight: bold;
            letter-spacing: 0.025em;
            line-height: 1.3;
        }
        h1 {
            font-size: 1.375rem;
            margin: 0.75rem 0 -0.25rem 0;
            padding-bottom: .3em;
            border-bottom: 1px solid )" + h1_line + R"(;
        }
        h2 {
            font-size: 1.125rem;
            margin: 0.75rem 0 -0.25rem 0;
            padding-bottom: .3em;
            border-bottom: 1px solid )" + h1_line + R"(;
        }
        h3 { font-size: 1rem; margin: 0.5rem 0 -0.25rem 0; }
        h4 { font-size: 1rem; font-weight: 600; margin: 0.5rem 0 -0.25rem 0; }

        p { margin: 0; line-height: 1.9; }

        strong { font-weight: 600; color: )" + strong_color + R"(; }
        em { font-style: italic; }

        a { color: )" + link + R"(; text-decoration: underline; text-underline-offset: 2px; }
        a:hover { opacity: 0.8; }

        hr {
            border: none;
            height: 4px;
            padding: 0;
            margin: 16px 0;
            background-color: )" + hr_bg + R"(;
            overflow: hidden;
            box-sizing: content-box;
        }

        blockquote {
            border-left: 4px solid )" + quote_border + R"(;
            padding: 0 15px;
            margin: 0 0;
            color: )" + quote_text + R"(;
        }
        blockquote p { padding: 0 2rem 0 0.5rem; }

        ul, ol {
            padding-left: 30px;
            margin: 0.25rem 0;
        }
        li { line-height: 1.65; margin: 2px 0; }

        .task-list { padding-left: 0; }
        .task-list-item { padding-left: 32px; }
        .task-list-item input[type="checkbox"] {
            top: 3px;
            left: 8px;
        }

        pre {
            margin-bottom: 15px;
            margin-top: 15px;
            padding: 0.2em 1em;
            padding-top: 8px;
            padding-bottom: 6px;
            background: )" + pre_bg + R"(;
            border: 1px solid )" + h1_line + R"(;
            border-radius: 3px;
            overflow-x: auto;
        }
        pre code {
            background: transparent;
            color: )" + pre_txt + R"(;
            padding: 0;
            font-size: 0.875rem;
            line-height: 1.625;
        }
        code {
            border: 1px solid )" + h1_line + R"(;
            background: )" + code_bg + R"(;
            color: )" + code_txt + R"(;
            border-radius: 3px;
            padding: 2px 4px;
            font-size: 0.9em;
            font-family: "JetBrains Mono", "Fira Code", ui-monospace, monospace;
        }

        table {
            padding: 0;
            border-collapse: collapse;
            width: 100%;
            margin: 0.8em 0;
            font-size: 0.875rem;
        }
        table tr:nth-child(2n) { background-color: )" + quote_bg + R"(; }
        table tr th, table tr td {
            margin: 0;
            padding: 0;
        }

        input[type="checkbox"] {
            accent-color: #4183C4;
            margin-right: 6px;
        }

        .empty-hint {
            color: )" + text_soft + R"(;
            line-height: 1.9;
        }

        del { text-decoration: line-through; color: )" + text_soft + R"(; }
    )";

    QString body;
    if (markdown.trimmed().isEmpty()) {
        body = "<p class=\"empty-hint\">预览区会显示当前笔记内容</p>";
    } else {
        body = md4cToHtml(markdown, h1_line, quote_bg);
        injectDataLineAttrs(body, markdown);
    }

    return "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
           "<style>" + css + "</style>"
           "</head><body><div class=\"md-content\">" + body + "</div></body></html>";
}
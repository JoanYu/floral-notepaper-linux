#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QTimer>
#include <QMap>
#include "services/notestore.h"
#include "services/configmanager.h"

class QSplitter;
class ClampedSplitter;
class QTextEdit;
class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class MarkdownPreview;
class SettingsPanel;
class SlidingButtonGroup;
class QPropertyAnimation;

class CategoryHeader : public QWidget {
	Q_OBJECT
public:
	CategoryHeader(const QString &name, bool isUncategorized, QWidget *parent);
	QString categoryName() const { return m_name; }
	void setExpanded(bool expanded);
	void setCount(int count);
protected:
	void mousePressEvent(QMouseEvent *e) override;
signals:
	void toggled(const QString &name);
	void renameRequested(const QString &name);
	void deleteRequested(const QString &name);
private:
	QString m_name;
	bool m_isUncategorized;
	bool m_expanded = true;
	QPushButton *m_arrow = nullptr;
	QPushButton *m_icon = nullptr;
	QLabel *m_label = nullptr;
	QLabel *m_count = nullptr;
};

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = nullptr);
	void initialize(NoteStore *store, ConfigManager *configMgr);

protected:
	void mousePressEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mouseDoubleClickEvent(QMouseEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	bool eventFilter(QObject *obj, QEvent *event) override;
	void changeEvent(QEvent *e) override;

private slots:
	void onNoteClicked(const QString &id);
	void onNewNote();
	void onSaveNote();
	void onDeleteNote();
	void onContentChanged();
	void onViewModeChanged(const QString &mode);
	void onSettingsChanged(const AppConfig &config);
	void toggleSettings();
	void toggleCategory(const QString &name);
	void onNewCategory();
	void onRenameCategory(const QString &oldName);
	void onDeleteCategory(const QString &name);
	void onFormatBtn(const QString &action);
	void onSearchChanged(const QString &text);
	void showMoveCategoryMenu(const QString &noteId);
	void moveNoteToCategory(const QString &noteId, const QString &category);
	void startRenameNote(const QString &noteId);
	void toggleSidebar();
	void onPinToTile();
	void onUndo();
	void onDeleteClicked();

private:
	void buildUI();
	QWidget *buildTitleBar();
	void rebuildNoteList();
	void loadNote(const QString &id);
	void applyConfig(const AppConfig &config);
	void updateStatusBar();
	void applyFormat(const QString &action);
	QString extractTitle(const QString &content) const;
	QWidget *makeNoteItem(const NoteMetadata &meta, bool inCategory = false);
	QWidget *makeCategoryGroup(const QString &cat, const QVector<NoteMetadata> &notes);
	void updateSubTitleElide();

	NoteStore *m_store = nullptr;
	ConfigManager *m_configMgr = nullptr;
	AppConfig m_config;
	QPoint m_dragPos;

	enum SaveState { Idle, Dirty, Saving, Saved, Error };
	SaveState m_saveState = Idle;
	QTimer *m_autoSaveTimer = nullptr;

	bool m_renaming = false;
	QString m_renameNoteId;
	QLineEdit *m_renameEdit = nullptr;

	bool m_renamingCat = false;
	QString m_renameCatName;
	QLineEdit *m_catRenameEdit = nullptr;

	bool m_syncingScroll = false;

	QScrollArea *m_sidebarScroll = nullptr;
	QWidget *m_sidebarContent = nullptr;
	QVBoxLayout *m_sidebarLayout = nullptr;
	QLineEdit *m_searchBox = nullptr;

	QTextEdit *m_editor = nullptr;
	MarkdownPreview *m_preview = nullptr;
	SettingsPanel *m_settingsPanel = nullptr;
	SlidingButtonGroup *m_viewModeGroup = nullptr;
	ClampedSplitter *m_splitter = nullptr;
	QWidget *m_editorContainer = nullptr;
	QWidget *m_previewContainer = nullptr;
	QLabel *m_lineCountLabel = nullptr;
	QLabel *m_byteSizeLabel = nullptr;
	QLineEdit *m_titleEdit = nullptr;
	QLabel *m_dateLabel = nullptr;
	QLabel *m_createdLabel = nullptr;
	QLabel *m_metaStatusLabel = nullptr;
	QLabel *m_charCountLabel = nullptr;
	QLabel *m_emptyState = nullptr;

	// Sidebar collapse & resize
	bool m_sidebarCollapsed = false;
	QWidget *m_sidebarWidget = nullptr;
	QPropertyAnimation *m_sidebarAnim = nullptr;
	int m_sidebarWidth = 280;
	QWidget *m_sidebarHandle = nullptr;
	bool m_resizingSidebar = false;
	int m_resizeStartX = 0;

	// Settings panel slide
	bool m_settingsOpen = false;
	QPropertyAnimation *m_settingsAnim = nullptr;

	// Toolbar buttons
	QLabel *m_subTitleLabel = nullptr;
	QPushButton *m_sidebarToggleBtn = nullptr;
	QPushButton *m_maxBtn = nullptr;


	QString m_selectedNoteId;
	bool m_dirty = false;
	QString m_currentViewMode = "edit";
	QString m_searchFilter;
	QMap<QString, QWidget*> m_noteItems;
	QMap<QString, CategoryHeader*> m_catHeaders;
};

#endif

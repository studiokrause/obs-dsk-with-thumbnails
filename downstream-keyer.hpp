#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QSlider>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolBar>
#include <QWidget>
#include <set>

#include "obs.h"
#include "obs-websocket-api.h"

typedef void (*get_transitions_callback_t)(void *data, struct obs_frontend_source_list *sources);

class LockedCheckBox : public QCheckBox {
	Q_OBJECT

public:
	LockedCheckBox();
	explicit LockedCheckBox(QWidget *parent);
};

// change: from one scene of the DSK to another of the same DSK
enum transitionType { match, show, hide, override, change };

// Scene list that does not scroll to the clicked scene, so a partly visible scene does not jump.
class DownstreamKeyerSceneList : public QListWidget {
private:
	bool clicking = false;

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

public:
	explicit DownstreamKeyerSceneList(QWidget *parent) : QListWidget(parent) {}
	void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
};

class DownstreamKeyer : public QWidget {
	Q_OBJECT

private:
	QTimer hideTimer;
	int outputChannel;
	obs_source_t *transition;
	obs_source_t *showTransition;
	obs_source_t *hideTransition;
	obs_source_t *overrideTransition;
	obs_source_t *changeTransition = nullptr;
	DownstreamKeyerSceneList *scenesList;
	QToolBar *scenesToolbar;
	uint32_t transitionDuration;
	uint32_t showTransitionDuration;
	uint32_t hideTransitionDuration;
	uint32_t overrideTransitionDuration = 300;
	uint32_t changeTransitionDuration = 300;
	uint32_t hideAfter = 0;
	LockedCheckBox *tie;
	obs_hotkey_id null_hotkey_id;
	obs_hotkey_pair_id tie_hotkey_id;
	std::set<std::string> exclude_scenes;
	obs_view_t *view = nullptr;
	obs_canvas_t *canvas = nullptr;
	get_transitions_callback_t get_transitions = nullptr;
	void *get_transitions_data = nullptr;
	bool showThumbnails = false;
	int thumbnailSize = 160;
	QTimer thumbnailTimer;
	bool thumbnailAutosize = false;
	QToolBar *thumbnailToolbar;
	QCheckBox *thumbnailAutosizeCheckBox;
	QSlider *thumbnailSlider;

	static void source_rename(void *data, calldata_t *calldata);
	static void source_remove(void *data, calldata_t *calldata);
	static bool enable_DSK_hotkey(void *data, obs_hotkey_pair_id id, obs_hotkey_t *hotkey, bool pressed);
	static bool disable_DSK_hotkey(void *data, obs_hotkey_pair_id id, obs_hotkey_t *hotkey, bool pressed);

	static void null_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed);

	static bool enable_tie_hotkey(void *data, obs_hotkey_pair_id id, obs_hotkey_t *hotkey, bool pressed);
	static bool disable_tie_hotkey(void *data, obs_hotkey_pair_id id, obs_hotkey_t *hotkey, bool pressed);

	void ChangeSceneIndex(bool relative, int idx, int invalidIdx);
	void ApplyViewMode();
	void UpdateThumbnailLayout();
	int GetAvailableWidth() const;
	bool NeedsVerticalScrollBar() const;
	int GetThumbnailSizeForWidth(int available) const;
	QSize GetCellSizeForThumbnail(int size) const;
	void UpdateThumbnails();

protected:
	bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
	void on_actionAddScene_triggered();
	void on_actionRemoveScene_triggered();
	void on_actionSceneUp_triggered();
	void on_actionSceneDown_triggered();
	void on_actionSceneNull_triggered();
	void apply_source(obs_source_t *newSource);
	void apply_selected_source();
	void on_scenesList_itemSelectionChanged();
signals:

public:
	DownstreamKeyer(int channel, QString name, obs_view_t *view = nullptr, obs_canvas_t *canvas = nullptr,
			get_transitions_callback_t get_transitions = nullptr, void *get_transitions_data = nullptr);
	~DownstreamKeyer();

	void Save(obs_data_t *data);
	void Load(obs_data_t *data);
	void SetTransition(const char *transition_name, enum transitionType transition_type = match);
	std::string GetTransition(enum transitionType transition_type = match);
	void SetTransitionDuration(int duration, enum transitionType transition_type = match);
	int GetTransitionDuration(enum transitionType transition_type = match);
	void SetHideAfter(int duration);
	int GetHideAfter();
	void SceneChanged(std::string scene);
	void AddExcludeScene(const char *scene_name);
	void RemoveExcludeScene(const char *scene_name);
	bool IsSceneExcluded(const char *scene_name);
	QString GetScene();
	bool SwitchToScene(QString scene_name);
	void add_scene(QString scene_name, obs_source_t *s, int insertBeforeRow);
	bool AddScene(QString scene_name, int insertBeforeRow);
	bool RemoveScene(QString scene_name);
	void SetTie(bool tie);
	void SetOutputChannel(int outputChannel);
	void SetThumbnails(bool enabled);
	bool GetThumbnails() const { return showThumbnails; }
	void SetThumbnailSize(int size);
	int GetThumbnailSize() const { return thumbnailSize; }
	void SetThumbnailAutosize(bool autosize);
	int GetEffectiveThumbnailSize() const;
	QSize GetThumbnailCellSize() const;
	double GetAspectRatio() const;
};

// Paints scenes either as a plain list or as thumbnails, with a red tally
// border around scenes that are visible in the program output.
class DownstreamKeyerItemDelegate : public QStyledItemDelegate {
	Q_OBJECT

private:
	DownstreamKeyer *keyer;

public:
	explicit DownstreamKeyerItemDelegate(DownstreamKeyer *keyer);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

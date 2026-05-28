#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QDockWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>
#include <QProgressBar>
#include <QString>
#include <QList>
#include <QMap>
#include <QPropertyAnimation>
#include <QPointer>
#include <string>
#include <vector>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-fade-dock", "en-US")

// ─────────────────────────────────────────────
//  SceneRow  — one row per scene
// ─────────────────────────────────────────────
class SceneRow : public QFrame {
	Q_OBJECT
public:
	QString sceneName;
	QLabel *nameLabel;
	QPushButton *btnIn;
	QPushButton *btnOut;
	QPushButton *btnSwitch;
	QProgressBar *fadeBar;
	QTimer *barTimer;

	explicit SceneRow(const QString &name, QWidget *parent = nullptr)
		: QFrame(parent), sceneName(name)
	{
		setObjectName("SceneRow");
		setFixedHeight(52);

		auto *outer = new QVBoxLayout(this);
		outer->setContentsMargins(6, 4, 6, 4);
		outer->setSpacing(3);

		// — top row: dot + name + buttons —
		auto *row = new QHBoxLayout();
		row->setSpacing(6);

		// live dot
		auto *dot = new QLabel("●");
		dot->setObjectName("LiveDot");
		dot->setFixedWidth(10);
		row->addWidget(dot);

		// scene name
		nameLabel = new QLabel(name);
		nameLabel->setObjectName("SceneName");
		nameLabel->setSizePolicy(QSizePolicy::Expanding,
					 QSizePolicy::Preferred);
		nameLabel->setToolTip(name);
		row->addWidget(nameLabel);

		// ▲ Fade In
		btnIn = new QPushButton("▲ In");
		btnIn->setObjectName("BtnIn");
		btnIn->setFixedSize(46, 22);
		btnIn->setToolTip("Fade in to this scene");
		row->addWidget(btnIn);

		// ▼ Fade Out
		btnOut = new QPushButton("▼ Out");
		btnOut->setObjectName("BtnOut");
		btnOut->setFixedSize(46, 22);
		btnOut->setToolTip("Fade out from this scene to black");
		row->addWidget(btnOut);

		// ⇌ Switch
		btnSwitch = new QPushButton("⇌");
		btnSwitch->setObjectName("BtnSwitch");
		btnSwitch->setFixedSize(28, 22);
		btnSwitch->setToolTip("Switch to this scene with fade");
		row->addWidget(btnSwitch);

		outer->addLayout(row);

		// — progress bar —
		fadeBar = new QProgressBar();
		fadeBar->setObjectName("FadeBar");
		fadeBar->setRange(0, 1000);
		fadeBar->setValue(0);
		fadeBar->setTextVisible(false);
		fadeBar->setFixedHeight(2);
		fadeBar->setVisible(false);
		outer->addWidget(fadeBar);

		barTimer = new QTimer(this);
		barTimer->setSingleShot(true);
	}

	void setActive(bool active)
	{
		setProperty("active", active);
		style()->unpolish(this);
		style()->polish(this);
		nameLabel->setProperty("active", active);
		nameLabel->style()->unpolish(nameLabel);
		nameLabel->style()->polish(nameLabel);
	}

	void animateFade(int durationMs, const QString &dir)
	{
		fadeBar->setProperty("direction", dir);
		fadeBar->style()->unpolish(fadeBar);
		fadeBar->style()->polish(fadeBar);
		fadeBar->setVisible(true);
		fadeBar->setValue(0);

		// animate via QTimer steps
		int steps = 60;
		int interval = durationMs / steps;
		int step = 0;

		auto *t = new QTimer(this);
		t->setInterval(interval);
		connect(t, &QTimer::timeout, [=]() mutable {
			step++;
			fadeBar->setValue(step * (1000 / steps));
			if (step >= steps) {
				t->stop();
				t->deleteLater();
				fadeBar->setVisible(false);
				fadeBar->setValue(0);
			}
		});
		t->start();
	}
};

// ─────────────────────────────────────────────
//  FadeDockWidget
// ─────────────────────────────────────────────
class FadeDockWidget : public QWidget {
	Q_OBJECT

	QSlider *durSlider;
	QLabel *durLabel;
	QWidget *listContainer;
	QVBoxLayout *listLayout;
	QMap<QString, SceneRow *> rows;
	int fadeDuration = 1000; // ms
	QTimer *refreshTimer;

	static const char *FTB_SCENE;

public:
	explicit FadeDockWidget(QWidget *parent = nullptr) : QWidget(parent)
	{
		setObjectName("FadeDockWidget");
		auto *root = new QVBoxLayout(this);
		root->setContentsMargins(0, 0, 0, 0);
		root->setSpacing(0);

		// ── Duration bar ──
		auto *durFrame = new QFrame();
		durFrame->setObjectName("DurFrame");
		durFrame->setFixedHeight(32);
		auto *durRow = new QHBoxLayout(durFrame);
		durRow->setContentsMargins(8, 0, 8, 0);
		durRow->setSpacing(6);

		auto *durTxt = new QLabel("Duration");
		durTxt->setObjectName("DurLabel");
		durRow->addWidget(durTxt);

		durSlider = new QSlider(Qt::Horizontal);
		durSlider->setRange(100, 5000);
		durSlider->setSingleStep(100);
		durSlider->setPageStep(500);
		durSlider->setValue(1000);
		durRow->addWidget(durSlider);

		durLabel = new QLabel("1.0 s");
		durLabel->setObjectName("DurVal");
		durLabel->setFixedWidth(34);
		durRow->addWidget(durLabel);

		root->addWidget(durFrame);

		// ── Divider ──
		auto *div = new QFrame();
		div->setFrameShape(QFrame::HLine);
		div->setObjectName("Divider");
		root->addWidget(div);

		// ── Scroll area for scene rows ──
		auto *scroll = new QScrollArea();
		scroll->setWidgetResizable(true);
		scroll->setFrameShape(QFrame::NoFrame);
		scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		listContainer = new QWidget();
		listContainer->setObjectName("ListContainer");
		listLayout = new QVBoxLayout(listContainer);
		listLayout->setContentsMargins(4, 4, 4, 4);
		listLayout->setSpacing(2);
		listLayout->addStretch();

		scroll->setWidget(listContainer);
		root->addWidget(scroll);

		// ── Signals ──
		connect(durSlider, &QSlider::valueChanged, this,
			[this](int v) {
				fadeDuration = v;
				durLabel->setText(
					QString::number(v / 1000.0, 'f', 1) +
					" s");
			});

		// ── OBS frontend callbacks ──
		obs_frontend_add_event_callback(onFrontendEvent, this);

		// ── Refresh timer (poll for scene changes) ──
		refreshTimer = new QTimer(this);
		refreshTimer->setInterval(500);
		connect(refreshTimer, &QTimer::timeout, this,
			&FadeDockWidget::refreshScenes);
		refreshTimer->start();

		// initial populate
		QTimer::singleShot(500, this, &FadeDockWidget::refreshScenes);
	}

	~FadeDockWidget()
	{
		obs_frontend_remove_event_callback(onFrontendEvent, this);
	}

private:
	static void onFrontendEvent(enum obs_frontend_event event, void *data)
	{
		auto *self = static_cast<FadeDockWidget *>(data);
		switch (event) {
		case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
		case OBS_FRONTEND_EVENT_FINISHED_LOADING:
			QMetaObject::invokeMethod(self, "refreshScenes",
						  Qt::QueuedConnection);
			break;
		case OBS_FRONTEND_EVENT_SCENE_CHANGED:
			QMetaObject::invokeMethod(self, "updateActiveScene",
						  Qt::QueuedConnection);
			break;
		default:
			break;
		}
	}

	void ensureFTBScene()
	{
		// Check if our black FTB scene exists, create if not
		obs_source_t *src =
			obs_get_source_by_name(FadeDockWidget::FTB_SCENE);
		if (src) {
			obs_source_release(src);
			return;
		}
		obs_scene_t *ftb = obs_scene_create(FadeDockWidget::FTB_SCENE);
		if (ftb)
			obs_scene_release(ftb);
	}

	void setFadeTransition()
	{
		// Find the "Fade" transition and set it as current
		obs_frontend_source_list tlist = {};
		obs_frontend_get_transitions(&tlist);

		for (size_t i = 0; i < tlist.sources.num; i++) {
			obs_source_t *tr = tlist.sources.array[i];
			const char *id = obs_source_get_id(tr);
			if (id && strcmp(id, "fade_transition") == 0) {
				obs_frontend_set_current_transition(tr);
				break;
			}
		}
		obs_frontend_source_list_free(&tlist);
		obs_frontend_set_transition_duration(fadeDuration);
	}

	void doFadeIn(const QString &name)
	{
		setFadeTransition();
		obs_source_t *src =
			obs_get_source_by_name(name.toUtf8().constData());
		if (src) {
			obs_frontend_set_current_scene(src);
			obs_source_release(src);
		}
		if (rows.contains(name))
			rows[name]->animateFade(fadeDuration, "in");
	}

	void doFadeOut(const QString &name)
	{
		ensureFTBScene();
		setFadeTransition();
		obs_source_t *src =
			obs_get_source_by_name(FadeDockWidget::FTB_SCENE);
		if (src) {
			obs_frontend_set_current_scene(src);
			obs_source_release(src);
		}
		if (rows.contains(name))
			rows[name]->animateFade(fadeDuration, "out");
	}

	void doSwitch(const QString &name)
	{
		setFadeTransition();
		obs_source_t *src =
			obs_get_source_by_name(name.toUtf8().constData());
		if (src) {
			obs_frontend_set_current_scene(src);
			obs_source_release(src);
		}
		if (rows.contains(name))
			rows[name]->animateFade(fadeDuration, "in");
	}

public slots:
	void refreshScenes()
	{
		obs_frontend_source_list slist = {};
		obs_frontend_get_scenes(&slist);

		QStringList names;
		for (size_t i = 0; i < slist.sources.num; i++) {
			const char *n =
				obs_source_get_name(slist.sources.array[i]);
			if (n && strcmp(n, FadeDockWidget::FTB_SCENE) != 0)
				names << QString::fromUtf8(n);
		}
		obs_frontend_source_list_free(&slist);

		// Remove stale rows
		QStringList existing = rows.keys();
		for (const QString &k : existing) {
			if (!names.contains(k)) {
				auto *row = rows.take(k);
				listLayout->removeWidget(row);
				row->deleteLater();
			}
		}

		// Add new rows
		int idx = 0;
		for (const QString &name : names) {
			if (!rows.contains(name)) {
				auto *row = new SceneRow(name, listContainer);

				connect(row->btnIn, &QPushButton::clicked, this,
					[this, name]() { doFadeIn(name); });
				connect(row->btnOut, &QPushButton::clicked, this,
					[this, name]() { doFadeOut(name); });
				connect(row->btnSwitch, &QPushButton::clicked,
					this,
					[this, name]() { doSwitch(name); });

				// insert before the stretch
				listLayout->insertWidget(idx, row);
				rows[name] = row;
			}
			idx++;
		}

		updateActiveScene();
	}

	void updateActiveScene()
	{
		obs_source_t *cur = obs_frontend_get_current_scene();
		QString curName;
		if (cur) {
			curName = QString::fromUtf8(obs_source_get_name(cur));
			obs_source_release(cur);
		}
		for (auto it = rows.begin(); it != rows.end(); ++it)
			it.value()->setActive(it.key() == curName);
	}
};

const char *FadeDockWidget::FTB_SCENE = "__FadeDock_FTB__";

// ─────────────────────────────────────────────
//  Plugin entry points
// ─────────────────────────────────────────────
static FadeDockWidget *gDockWidget = nullptr;
static QDockWidget *gDock = nullptr;

bool obs_module_load()
{
	// Build UI on the main thread after OBS finishes loading
	obs_frontend_add_event_callback(
		[](enum obs_frontend_event event, void *) {
			if (event != OBS_FRONTEND_EVENT_FINISHED_LOADING)
				return;

			auto *mainWin = static_cast<QMainWindow *>(
				obs_frontend_get_main_window());

			gDockWidget = new FadeDockWidget(mainWin);

			gDock = new QDockWidget(
				QObject::tr("Scene Fade Controls"), mainWin);
			gDock->setObjectName("FadeDockWidget");
			gDock->setWidget(gDockWidget);
			gDock->setFeatures(QDockWidget::DockWidgetMovable |
					   QDockWidget::DockWidgetFloatable |
					   QDockWidget::DockWidgetClosable);
			gDock->setMinimumWidth(220);

			mainWin->addDockWidget(Qt::RightDockWidgetArea, gDock);
			gDock->show();

			// Stylesheet — matches OBS dark theme
			gDockWidget->setStyleSheet(R"(
#FadeDockWidget {
	background: #1a1a1f;
}
#DurFrame {
	background: #22222a;
	border-bottom: 1px solid rgba(255,255,255,0.07);
}
#DurLabel {
	font-size: 11px;
	color: #7a7a90;
}
#DurVal {
	font-size: 11px;
	font-weight: bold;
	color: #e8e8f0;
}
#Divider {
	color: rgba(255,255,255,0.07);
}
#ListContainer {
	background: #1a1a1f;
}
#SceneRow {
	background: #22222a;
	border: 1px solid rgba(255,255,255,0.07);
	border-radius: 5px;
}
#SceneRow[active="true"] {
	background: rgba(91,111,255,0.15);
	border: 1px solid #5b6fff;
}
#LiveDot {
	color: transparent;
	font-size: 8px;
}
#SceneRow[active="true"] #LiveDot {
	color: #5b6fff;
}
#SceneName {
	font-size: 12px;
	color: #e8e8f0;
}
#SceneRow[active="true"] #SceneName {
	color: #ffffff;
	font-weight: bold;
}
#BtnIn {
	background: rgba(62,207,142,0.12);
	border: 1px solid rgba(62,207,142,0.3);
	border-radius: 4px;
	color: #3ecf8e;
	font-size: 10px;
	font-weight: bold;
	padding: 0px;
}
#BtnIn:hover {
	background: rgba(62,207,142,0.25);
}
#BtnIn:pressed {
	background: rgba(62,207,142,0.35);
}
#BtnOut {
	background: rgba(255,92,92,0.12);
	border: 1px solid rgba(255,92,92,0.3);
	border-radius: 4px;
	color: #ff5c5c;
	font-size: 10px;
	font-weight: bold;
	padding: 0px;
}
#BtnOut:hover {
	background: rgba(255,92,92,0.25);
}
#BtnOut:pressed {
	background: rgba(255,92,92,0.35);
}
#BtnSwitch {
	background: rgba(91,111,255,0.12);
	border: 1px solid rgba(91,111,255,0.3);
	border-radius: 4px;
	color: #5b6fff;
	font-size: 12px;
	padding: 0px;
}
#BtnSwitch:hover {
	background: rgba(91,111,255,0.25);
}
#BtnSwitch:pressed {
	background: rgba(91,111,255,0.35);
}
#FadeBar {
	border-radius: 1px;
	border: none;
	background: rgba(255,255,255,0.07);
}
#FadeBar[direction="in"]::chunk {
	background: #3ecf8e;
	border-radius: 1px;
}
#FadeBar[direction="out"]::chunk {
	background: #ff5c5c;
	border-radius: 1px;
}
QScrollArea {
	background: #1a1a1f;
	border: none;
}
QScrollBar:vertical {
	background: transparent;
	width: 4px;
}
QScrollBar::handle:vertical {
	background: rgba(255,255,255,0.15);
	border-radius: 2px;
}
QSlider::groove:horizontal {
	height: 3px;
	background: rgba(255,255,255,0.15);
	border-radius: 1px;
}
QSlider::handle:horizontal {
	background: #5b6fff;
	width: 10px;
	height: 10px;
	margin: -4px 0;
	border-radius: 5px;
}
QSlider::sub-page:horizontal {
	background: #5b6fff;
	border-radius: 1px;
}
)");
		},
		nullptr);

	return true;
}

void obs_module_unload()
{
	if (gDock) {
		gDock->deleteLater();
		gDock = nullptr;
		gDockWidget = nullptr;
	}
}

#include "fade-dock.moc"

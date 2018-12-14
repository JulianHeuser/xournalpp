#include "SettingsDialog.h"

#include "ButtonConfigGui.h"
#include "gui/widgets/ZoomCallib.h"

#include <config.h>
#include <Util.h>
#include <i18n.h>

#include <string.h>

SettingsDialog::SettingsDialog(GladeSearchpath* gladeSearchPath, Settings* settings)
 : GladeGui(gladeSearchPath, "settings.glade", "settingsDialog"),
   settings(settings),
   callib(zoomcallib_new()),
   dpi(72)
{
	XOJ_INIT_TYPE(SettingsDialog);

	GtkWidget* vbox = get("zoomVBox");
	g_return_if_fail(vbox != NULL);

	GtkWidget* slider = get("zoomCallibSlider");
	g_return_if_fail(slider != NULL);

	g_signal_connect(slider, "change-value", G_CALLBACK(
		+[](GtkRange* range, GtkScrollType scroll, gdouble value, SettingsDialog* self)
		{
			XOJ_CHECK_TYPE_OBJ(self, SettingsDialog);
			self->setDpi((int) value);
		}), this);

	g_signal_connect(get("cbAutosave"), "toggled", G_CALLBACK(
		+[](GtkToggleButton* togglebutton, SettingsDialog* self)
		{
			XOJ_CHECK_TYPE_OBJ(self, SettingsDialog);
			self->autosaveToggled();
		}), this);

	gtk_box_pack_start(GTK_BOX(vbox), callib, false, true, 0);
	gtk_widget_show(callib);

	initMouseButtonEvents();
	initAdvanceInputConfig();
}

SettingsDialog::~SettingsDialog()
{
	XOJ_CHECK_TYPE(SettingsDialog);

	for (ButtonConfigGui* bcg : this->buttonConfigs)
	{
		delete bcg;
	}

	// DO NOT delete settings!
	this->settings = NULL;

	XOJ_RELEASE_TYPE(SettingsDialog);
}

void SettingsDialog::initAdvanceInputConfig()
{
	GtkComboBoxText* cbSelectInputType = GTK_COMBO_BOX_TEXT(get("cbSelectInputType"));
	gtk_combo_box_text_append_text(cbSelectInputType, _C("Auto"));
	gtk_combo_box_text_append_text(cbSelectInputType, _C("01: GTK / may crash on X11"));
	gtk_combo_box_text_append_text(cbSelectInputType, _C("02: Direct read axes"));

	// Not fully working yet
	// gtk_combo_box_text_append_text(cbSelectInputType, _C("03: GTK multi device support"));

	string selectedInputType;
	SElement& inputSettings = settings->getCustomElement("inputHandling");
	inputSettings.getString("type", selectedInputType);

	if (selectedInputType == "01-gtk")
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(cbSelectInputType), 1);
	}
	else if (selectedInputType == "02-direct")
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(cbSelectInputType), 2);
	}
	else if (selectedInputType == "03-gtk")
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(cbSelectInputType), 3);
	}
	else // selectedInputType == "auto"
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(cbSelectInputType), 0);
	}
}

void SettingsDialog::initMouseButtonEvents(const char* hbox, int button, bool withDevice)
{
	XOJ_CHECK_TYPE(SettingsDialog);

	this->buttonConfigs.push_back(new ButtonConfigGui(this, getGladeSearchPath(), get(hbox),settings, button, withDevice));
}

void SettingsDialog::initMouseButtonEvents()
{
	XOJ_CHECK_TYPE(SettingsDialog);

	initMouseButtonEvents("hboxMidleMouse", 1);
	initMouseButtonEvents("hboxRightMouse", 2);
	initMouseButtonEvents("hboxEraser", 0);
	initMouseButtonEvents("hboxTouch", 3, true);
	initMouseButtonEvents("hboxPenButton1", 5);
	initMouseButtonEvents("hboxPenButton2", 6);

	initMouseButtonEvents("hboxDefault", 4);
}

void SettingsDialog::setDpi(int dpi)
{
	XOJ_CHECK_TYPE(SettingsDialog);

	if (this->dpi == dpi)
	{
		return;
	}

	this->dpi = dpi;
	zoomcallib_set_val(ZOOM_CALLIB(callib), dpi);
}

void SettingsDialog::show(GtkWindow* parent)
{
	XOJ_CHECK_TYPE(SettingsDialog);

	load();

	gtk_window_set_transient_for(GTK_WINDOW(this->window), parent);
	int res = gtk_dialog_run(GTK_DIALOG(this->window));

	if (res == 1)
	{
		this->save();
	}

	gtk_widget_hide(this->window);
}

void SettingsDialog::loadCheckbox(const char* name, gboolean value)
{
	XOJ_CHECK_TYPE(SettingsDialog);

	GtkToggleButton* b = GTK_TOGGLE_BUTTON(get(name));
	gtk_toggle_button_set_active(b, value);
}

bool SettingsDialog::getCheckbox(const char* name)
{
	XOJ_CHECK_TYPE(SettingsDialog);

	GtkToggleButton* b = GTK_TOGGLE_BUTTON(get(name));
	return gtk_toggle_button_get_active(b);
}

/**
 * Autosave was toggled, enable / disable autosave config
 */
void SettingsDialog::autosaveToggled()
{
	XOJ_CHECK_TYPE(SettingsDialog);

	GtkWidget* cbAutosave = get("cbAutosave");
	bool autosaveEnabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cbAutosave));
	gtk_widget_set_sensitive(get("lbAutosaveTimeout"), autosaveEnabled);
	gtk_widget_set_sensitive(get("spAutosaveTimeout"), autosaveEnabled);
}

void SettingsDialog::load()
{
	XOJ_CHECK_TYPE(SettingsDialog);

	loadCheckbox("cbSettingPresureSensitivity", settings->isPresureSensitivity());
	loadCheckbox("cbShowSidebarRight", settings->isSidebarOnRight());
	loadCheckbox("cbShowScrollbarLeft", settings->isScrollbarOnLeft());
	loadCheckbox("cbAutoloadXoj", settings->isAutloadPdfXoj());
	loadCheckbox("cbAutosave", settings->isAutosaveEnabled());
	loadCheckbox("cbAddVerticalSpace", settings->getAddVerticalSpace());
	loadCheckbox("cbAddHorizontalSpace", settings->getAddHorizontalSpace());
	loadCheckbox("cbBigCursor", settings->isShowBigCursor());
	loadCheckbox("cbHideHorizontalScrollbar", settings->getScrollbarHideType() & SCROLLBAR_HIDE_HORIZONTAL);
	loadCheckbox("cbHideVerticalScrollbar", settings->getScrollbarHideType() & SCROLLBAR_HIDE_VERTICAL);

	GtkWidget* txtDefaultSaveName = get("txtDefaultSaveName");
	string txt = settings->getDefaultSaveName();
	gtk_entry_set_text(GTK_ENTRY(txtDefaultSaveName), txt.c_str());

	gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(get("fcAudioPath")), settings->getAudioFolder().c_str());

	GtkWidget* spAutosaveTimeout = get("spAutosaveTimeout");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spAutosaveTimeout), settings->getAutosaveTimeout());

	GtkWidget* slider = get("zoomCallibSlider");

	this->setDpi(settings->getDisplayDpi());
	gtk_range_set_value(GTK_RANGE(slider), dpi);

	GdkRGBA color;
	Util::apply_rgb_togdkrgba(color, settings->getSelectionColor());
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(get("colorBorder")), &color);
	Util::apply_rgb_togdkrgba(color, settings->getBackgroundColor());
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(get("colorBackground")), &color);


	bool hideFullscreenMenubar = false;
	bool hideFullscreenSidebar = false;
	bool hidePresentationMenubar = false;
	bool hidePresentationSidebar = false;

	string hidden = settings->getFullscreenHideElements();
	StringTokenizer tokenF(hidden, ',');
	const char* element = tokenF.next();
	while (element)
	{
		if (!strcmp("mainMenubar", element))
		{
			hideFullscreenMenubar = true;
		}
		else if (!strcmp("sidebarContents", element))
		{
			hideFullscreenSidebar = true;
		}
		element = tokenF.next();
	}

	hidden = settings->getPresentationHideElements();
	StringTokenizer token(hidden, ',');
	element = token.next();
	while (element)
	{
		if (!strcmp("mainMenubar", element))
		{
			hidePresentationMenubar = true;
		}
		else if (!strcmp("sidebarContents", element))
		{
			hidePresentationSidebar = true;
		}
		element = token.next();
	}

	loadCheckbox("cbHideFullscreenMenubar", hideFullscreenMenubar);
	loadCheckbox("cbHideFullscreenSidebar", hideFullscreenSidebar);
	loadCheckbox("cbHidePresentationMenubar", hidePresentationMenubar);
	loadCheckbox("cbHidePresentationSidebar", hidePresentationSidebar);

	autosaveToggled();
}

string SettingsDialog::updateHideString(string hidden, bool hideMenubar, bool hideSidebar)
{
	XOJ_CHECK_TYPE(SettingsDialog);

	string newHidden = "";

	const char* element;
	StringTokenizer token(hidden, ',');
	element = token.next();
	while (element)
	{
		if (!strcmp("mainMenubar", element))
		{
			if (hideMenubar)
			{
				hideMenubar = false;
			}
			else
			{
				element = token.next();
				continue;
			}
		}
		else if (!strcmp("sidebarContents", element))
		{
			if (hideSidebar)
			{
				hideSidebar = false;
			}
			else
			{
				element = token.next();
				continue;
			}
		}

		if (!newHidden.empty())
		{
			newHidden += ",";
		}
		newHidden += element;

		element = token.next();
	}

	if (hideMenubar)
	{
		if (!newHidden.empty())
		{
			newHidden += ",";
		}
		newHidden += "mainMenubar";
	}

	if (hideSidebar)
	{
		if (!newHidden.empty())
		{
			newHidden += ",";
		}
		newHidden += "sidebarContents";
	}

	return newHidden;
}

void SettingsDialog::save()
{
	XOJ_CHECK_TYPE(SettingsDialog);

	settings->transactionStart();

	settings->setPresureSensitivity(getCheckbox("cbSettingPresureSensitivity"));
	settings->setSidebarOnRight(getCheckbox("cbShowSidebarRight"));
	settings->setScrollbarOnLeft(getCheckbox("cbShowScrollbarLeft"));
	settings->setAutoloadPdfXoj(getCheckbox("cbAutoloadXoj"));
	settings->setAutosaveEnabled(getCheckbox("cbAutosave"));
	settings->setAddVerticalSpace(getCheckbox("cbAddVerticalSpace"));
	settings->setAddHorizontalSpace(getCheckbox("cbAddHorizontalSpace"));
	settings->setShowBigCursor(getCheckbox("cbBigCursor"));

	int scrollbarHideType = SCROLLBAR_HIDE_NONE;
	if (getCheckbox("cbHideHorizontalScrollbar"))
	{
		scrollbarHideType |= SCROLLBAR_HIDE_HORIZONTAL;
	}
	if (getCheckbox("cbHideVerticalScrollbar"))
	{
		scrollbarHideType |= SCROLLBAR_HIDE_VERTICAL;
	}
	settings->setScrollbarHideType((ScrollbarHideType)scrollbarHideType);

	GdkRGBA color;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(get("colorBorder")), &color);
	settings->setSelectionColor(Util::gdkrgba_to_hex(color));

	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(get("colorBackground")), &color);
	settings->setBackgroundColor(Util::gdkrgba_to_hex(color));


	bool hideFullscreenMenubar = getCheckbox("cbHideFullscreenMenubar");
	bool hideFullscreenSidebar = getCheckbox("cbHideFullscreenSidebar");
	settings->setFullscreenHideElements(
			updateHideString(settings->getFullscreenHideElements(), hideFullscreenMenubar, hideFullscreenSidebar));

	bool hidePresentationMenubar = getCheckbox("cbHidePresentationMenubar");
	bool hidePresentationSidebar = getCheckbox("cbHidePresentationSidebar");
	settings->setPresentationHideElements(
			updateHideString(settings->getPresentationHideElements(), hidePresentationMenubar,
					hidePresentationSidebar));

	settings->setDefaultSaveName(gtk_entry_get_text(GTK_ENTRY(get("txtDefaultSaveName"))));
	char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(get("fcAudioPath")));
	if (uri != NULL)
	{
		settings->setAudioFolder(uri);
		g_free(uri);
	}

	GtkWidget* spAutosaveTimeout = get("spAutosaveTimeout");
	int autosaveTimeout = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spAutosaveTimeout));
	settings->setAutosaveTimeout(autosaveTimeout);

	settings->setDisplayDpi(dpi);

	for (ButtonConfigGui* bcg : this->buttonConfigs)
	{
		bcg->saveSettings();
	}

	SElement& inputSettings = settings->getCustomElement("inputHandling");
	GtkComboBox* cbSelectInputType = GTK_COMBO_BOX(get("cbSelectInputType"));
	int activeInput = gtk_combo_box_get_active(cbSelectInputType);

	if (activeInput == 1)
	{
		inputSettings.setString("type", "01-gtk");
	}
	else if (activeInput == 2)
	{
		inputSettings.setString("type", "02-direct");
	}
	else if (activeInput == 3)
	{
		inputSettings.setString("type", "03-gtk");
	}
	else
	{
		inputSettings.setString("type", "auto");
	}


	settings->transactionEnd();
}

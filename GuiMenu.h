#pragma once

#include "GraphicalSimulationEngine.h"
#include "MapData.h"
#include "TrainList.h"
#include "UiKit.h"

#include <SFML/Graphics.hpp>
#include <string>

class StationList;

enum class AppScreen { Stations, Trains, Maintenance, Map, Chart };

enum class FormKind {
    None, AddStation, AddTrain, AddCarriage, AddStop, EditStop, CloseResource, ReportDelay, Confirm
};

enum class ConfirmAction { None, RemoveTrain, RemoveStop, ReopenWindow };

class RailwayApp {
public:
    RailwayApp(StationList& stationList, TrainList& trainList);

    void init();
    void handleEvent(const sf::Event& event) { ui.feed(event); }
    void setMouse(sf::Vector2i pixel) { ui.setMouse(pixel); }
    bool wantsHand() const { return ui.wantsHand(); }

    // Vẽ và xử lý một khung hình; trả về false khi người dùng thoát.
    bool frame(sf::RenderTarget& target, float dt);

    void showScreen(AppScreen next) { screen = next; }
    void selectTrain(const std::string& code) { selectedTrain = code; }
    void openForm(FormKind kind);
    void openEditStop(const std::string& code, int stationID);

private:
    StationList& stations;
    TrainList& trains;
    TrainList baseline;
    Ui::Canvas ui;
    MapData map;
    SimModel model;
    GraphicalSimulationEngine simulation;

    AppScreen screen;
    bool running;
    std::string notice;
    bool noticeIsError;
    float noticeTime;

    std::string stationSearch;
    std::string trainSearch;
    std::string stopSearch;
    std::string selectedTrain;
    int stationScroll;
    int trainScroll;
    int windowScroll;

    FormKind formKind;
    std::string formValues[5];
    int formChoiceA;
    int formChoiceB;
    int formTrack;
    int formDayA;
    int formDayB;
    int formStation;
    std::string formTrain;
    std::string formError;
    std::string confirmText;
    ConfirmAction confirmAction;
    int confirmWindow;

    void refresh();
    void setNotice(const std::string& text, bool isError = false);
    bool saveAll();

    void scanConflicts();
    void reloadData();
    void applyCommands();
    bool changeMaintenance(bool opening, const std::string& resource, int first, int second,
                           int start, int end, std::string& error);

    void drawHeader();
    void drawStations();
    void drawTrains();
    void drawMaintenance();
    void drawChart();
    void drawLog(float x, float y, float width);
    void drawNotice();

    void closeForm();
    void drawForm();
    void submitForm();
    void runConfirm();
};

class GuiMenu {
public:
    static void runGUIMenu(StationList& stationList, TrainList& trainList);
};

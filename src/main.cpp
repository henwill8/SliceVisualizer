#include "../include/main.hpp"
#include "../include/codegen.hpp"
#include "../include/UI.hpp"
#include "../include/sprites.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "../extern/questui/shared/QuestUI.hpp"
#include "../extern/questui/shared/BeatSaberUI.hpp"
#include "../extern/custom-types/shared/register.hpp"

#include <sstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <vector>
#include <list>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <limits>
#include <math.h>
#include <fstream>
#include <string.h>

using namespace UnityEngine::UI;
using namespace il2cpp_utils;
using namespace GlobalNamespace;

static float clamp(float num, float min, float max) {
    if(num < min) {
        return min;
    } else if(num > max) {
        return max;
    } else {
        return num;
    }
}

Logger& logger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, false));
    return *logger;
}

#define log(...) logger().info(__VA_ARGS__)

Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

static void SaveConfig() {
    if (!getConfig().config.HasMember("DynamicFadeSpeed")) {
        // log("Regenerating config!");
        getConfig().config.SetObject();
        auto& allocator = getConfig().config.GetAllocator();
        
        getConfig().config.AddMember("Enabled", true, allocator);
        getConfig().config.AddMember("FadeSpeed", 10, allocator);
        getConfig().config.AddMember("DynamicFadeSpeed", true, allocator);

        getConfig().Write();
        // log("Config regeneration complete!");
    }
    else {
        // log("Not regnerating config.");
    }
}

template<class T>
UnityEngine::GameObject* FindObject(std::string name, bool byParent = true, bool getLastIndex = false) {
    // log("Finding GameObject of name %s", name);
    Array<T>* trs = UnityEngine::Resources::FindObjectsOfTypeAll<T>();
    // log("There are "+std::to_string(trs->Length())+" GameObjects");
    for(int i = 0; i < trs->Length(); i++) {
        if(i != trs->Length()-1 && getLastIndex) continue;
        UnityEngine::GameObject* go = trs->values[i]->get_gameObject();
        if(byParent) {
            go = go->get_transform()->GetParent()->get_gameObject();
        }
        // log(to_utf8(csstrtostr(UnityEngine::Transform::GetName(trs->values[i]))));
        if(to_utf8(csstrtostr(UnityEngine::Transform::GetName(go))) == name){
            // log("Found GameObject");
            return go;
        }
    }
    // log("Could not find GameObject");
    return nullptr;
}

void SetGlobalScale(UnityEngine::Transform* transform, UnityEngine::Vector3 globalScale) {
    transform->set_localScale(UnityEngine::Vector3::get_one());
    transform->set_localScale(UnityEngine::Vector3{globalScale.x/transform->get_lossyScale().x, globalScale.y/transform->get_lossyScale().y, globalScale.z/transform->get_lossyScale().z});
}

float distanceToCenter;

UnityEngine::Sprite* arrowSprite;
UnityEngine::Sprite* dotSprite;
UnityEngine::Sprite* arrowBackgroundSprite;
UnityEngine::Sprite* dotBackgroundSprite;

UnityEngine::GameObject* sliceVisualizerGO;

UnityEngine::Color leftColor;
UnityEngine::Color rightColor;

BeatmapObjectSpawnController* spawnController;

std::vector<std::pair<std::pair<std::pair<UnityEngine::UI::Image*, UnityEngine::UI::Image*>, UnityEngine::UI::Image*>, float>> cuts;

float nextNoteTime;

float spriteSize = 0.6f;
bool enabled = true;
float fadeSpeed;
bool dynamicFadeSpeed = true;
bool cutHasBeenMade = false;

UnityEngine::Material* uiMat;

void createSlice(ByRef<NoteCutInfo> noteCutInfo, NoteController* noteController, float distanceToCenter) {
    cutHasBeenMade = true;

    if(spawnController == nullptr) {
        spawnController = FindObject<BeatmapObjectSpawnController*>("BeatmapObjectSpawnController", false)->GetComponent<BeatmapObjectSpawnController*>();
    }

    // log("Making sprite Image");
    UnityEngine::GameObject* sliceGraphics = UnityEngine::GameObject::New_ctor(createcsstr("SliceGraphics"));
    sliceGraphics->get_transform()->SetParent(sliceVisualizerGO->get_transform(), false);
    sliceGraphics->get_transform()->set_localEulerAngles(UnityEngine::Vector3{0, 0, NoteCutDirectionExtensions::RotationAngle(noteController->get_noteData()->get_cutDirection())});
    UnityEngine::Vector3 position = sliceGraphics->get_transform()->get_position();
    UnityEngine::Vector2 notePosition = spawnController->beatmapObjectSpawnMovementData->Get2DNoteOffset(noteController->noteData->lineIndex, noteController->noteData->noteLineLayer);
    sliceGraphics->get_transform()->set_position(UnityEngine::Vector3{notePosition.x, position.y + notePosition.y, position.z});
    sliceGraphics->get_transform()->set_localScale(UnityEngine::Vector3{spriteSize, spriteSize, spriteSize});

    UnityEngine::GameObject* spriteGO = UnityEngine::GameObject::New_ctor(createcsstr("SpriteImage"));
    spriteGO->get_transform()->SetParent(sliceGraphics->get_transform(), false);
    spriteGO->get_transform()->set_localPosition(UnityEngine::Vector3{-0.0f/spriteSize, -0.0f/spriteSize, 0});
    UnityEngine::UI::Image* sprite = spriteGO->AddComponent<UnityEngine::UI::Image*>();
    sprite->set_material(uiMat);
    ColorType colorType = noteController->noteData->colorType;
    if(colorType == ColorType::ColorA) {
        sprite->set_color(leftColor);
    } else {
        sprite->set_color(rightColor);
    }
    UnityEngine::UI::Mask* spriteMask = spriteGO->AddComponent<UnityEngine::UI::Mask*>();
    spriteMask->set_showMaskGraphic(true);

    UnityEngine::GameObject* spriteArrowGO = UnityEngine::GameObject::New_ctor(createcsstr("SpriteArrowImage"));
    spriteArrowGO->get_transform()->SetParent(spriteGO->get_transform(), false);
    spriteArrowGO->get_transform()->set_localPosition(UnityEngine::Vector3{0, 0, 0});
    UnityEngine::UI::Image* spriteArrow = spriteArrowGO->AddComponent<UnityEngine::UI::Image*>();
    spriteArrow->set_material(uiMat);

    if(noteController->noteData->cutDirection == NoteCutDirection::Any) {
        sprite->set_sprite(dotBackgroundSprite);
        spriteArrow->set_sprite(dotSprite);
    } else {
        sprite->set_sprite(arrowBackgroundSprite);
        spriteArrow->set_sprite(arrowSprite);
    }

    // log("Creating line");
    UnityEngine::GameObject* lineGO = UnityEngine::GameObject::New_ctor(createcsstr("CutLine"));
    lineGO->get_transform()->SetParent(spriteGO->get_transform(), false);
    lineGO->get_transform()->set_localScale(UnityEngine::Vector3{1.0f/spriteSize, 1.0f/spriteSize, 1.0f/spriteSize});
    UnityEngine::UI::Image* lineSprite = lineGO->AddComponent<UnityEngine::UI::Image*>();
    lineSprite->set_color(UnityEngine::Color{0, 0, 0, 1});
    lineSprite->set_material(uiMat);
    UnityEngine::RectTransform* lineRT = lineGO->GetComponent<UnityEngine::RectTransform*>();
    lineRT->set_sizeDelta(UnityEngine::Vector2{5, 300});
    
    lineGO->get_transform()->set_localEulerAngles(UnityEngine::Vector3{0, 0, noteCutInfo.heldRef.cutDirDeviation});
    lineGO->get_transform()->set_localPosition(UnityEngine::Vector3{distanceToCenter*120/spriteSize - 2.3f/spriteSize, -2.3f/spriteSize, 0});

    // log("Adding slice to the cut vector");
    cuts.push_back(std::make_pair(std::make_pair(std::make_pair(sprite, spriteArrow), lineGO->GetComponent<UnityEngine::UI::Image*>()), 0.99f));
    // log("Successfully added the slice");
}

MAKE_HOOK_MATCH(SongUpdate, &AudioTimeSyncController::Update, void, AudioTimeSyncController* self) {
    if(cuts.size() > 0 && cutHasBeenMade) {
        for(int i = cuts.size()-1; i >= 0; i--) {
            float dynamicMultiplier = 1;
            if(dynamicFadeSpeed) dynamicMultiplier = clamp(2 - (nextNoteTime * 1.3f), 0.4f, 2.0f);
            float opacity = cuts[i].second - (std::min(0.4f, 1 - cuts[i].second) * UnityEngine::Time::get_deltaTime() * fadeSpeed * dynamicMultiplier);
            if(cuts[i].first.first.first->m_CachedPtr.m_value != nullptr) {
                UnityEngine::GameObject* go = cuts[i].first.first.first->get_gameObject();
                
                UnityEngine::Color color = cuts[i].first.first.first->get_color();
                color.a = opacity;
                cuts[i].first.first.first->set_color(color);
                cuts[i].first.first.second->set_color(UnityEngine::Color{1, 1, 1, opacity});
                cuts[i].first.second->set_color(UnityEngine::Color{0, 0, 0, opacity*2.2f});
                cuts[i].second = opacity;
                if(opacity < 0) {
                    // log("Destroying slice as its opacity has passed 0");
                    cuts.erase(cuts.begin()+i);
                    UnityEngine::Object::Destroy(go->get_transform()->GetParent()->get_gameObject());
                    continue;
                }
            }
        }
    }

    SongUpdate(self);
}

MAKE_HOOK_MATCH(SongStart, &AudioTimeSyncController::StartSong, void, AudioTimeSyncController* self, float startTimeOffset) {

    SongStart(self, startTimeOffset);

    uiMat = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>(), [](UnityEngine::Material* x) { return to_utf8(csstrtostr(x->get_name())) == "UINoGlow"; });

    enabled = getConfig().config["Enabled"].GetBool();
    fadeSpeed = getConfig().config["FadeSpeed"].GetFloat();
    dynamicFadeSpeed = getConfig().config["DynamicFadeSpeed"].GetBool();

    if(cuts.size() > 0) {
        for(int i = cuts.size()-1; i >= 0; i--) {
            if(cuts[i].first.first.first->m_CachedPtr.m_value != nullptr) {
                UnityEngine::GameObject* go = cuts[i].first.first.first->get_gameObject();
                cuts.erase(cuts.begin()+i);
                UnityEngine::Object::Destroy(go->get_transform()->GetParent()->get_gameObject());
                continue;
            }
        }
    }

    cuts.clear();
    cutHasBeenMade = false;

    // log("Getting arrow sprite");
    arrowSprite = QuestUI::BeatSaberUI::Base64ToSprite(arrowBase64, 1024, 1024);
    // log("Getting dot sprite");
    dotSprite = QuestUI::BeatSaberUI::Base64ToSprite(dotBase64, 1024, 1024);
    // log("Getting arrow background sprite");
    arrowBackgroundSprite = QuestUI::BeatSaberUI::Base64ToSprite(arrowBackgroundBase64, 1024, 1024);
    // log("Getting dot background sprite");
    dotBackgroundSprite = QuestUI::BeatSaberUI::Base64ToSprite(dotBackgroundBase64, 1024, 1024);

    sliceVisualizerGO = UnityEngine::GameObject::New_ctor(createcsstr("SliceVisualizerGO"));
    bool getLastIndex = false;
    if(FindObject<MultiplayerController*>("MultiplayerController", false) == nullptr) {
        getLastIndex = true;
    }
    // log("Get last index is %i", getLastIndex);
    sliceVisualizerGO->get_transform()->SetParent(FindObject<ComboUIController*>("ComboPanel", false, getLastIndex)->get_transform());
    sliceVisualizerGO->get_transform()->set_position(UnityEngine::Vector3{0, 3, 15});
    sliceVisualizerGO->get_transform()->set_eulerAngles(UnityEngine::Vector3{0, 0, 0});
    SetGlobalScale(sliceVisualizerGO->get_transform(), UnityEngine::Vector3{0.01f, 0.01f, 0.01f});

    spawnController = nullptr;
}

MAKE_HOOK_MATCH(NoteController_SendNoteWasCutEvent, &NoteController::SendNoteWasCutEvent, void, NoteController* self, ByRef<NoteCutInfo> noteCutInfo) {

    if(noteCutInfo.heldRef.get_allIsOK() && enabled) {
        if(dynamicFadeSpeed) {
            nextNoteTime = self->noteData->timeToNextColorNote;
        }
        createSlice(noteCutInfo, self, distanceToCenter);
    }

    NoteController_SendNoteWasCutEvent(self, noteCutInfo);
}

MAKE_HOOK_MATCH(GameNoteController_HandleBigWasCutBySaber, &GameNoteController::HandleBigWasCutBySaber, void, GameNoteController* self, Saber* saber, UnityEngine::Vector3 cutPoint, UnityEngine::Quaternion orientation, UnityEngine::Vector3 cutDirVec) {
    
    UnityEngine::Vector3 vector = orientation * UnityEngine::Vector3::get_up();
    UnityEngine::Plane plane = {vector, cutPoint};
    distanceToCenter = plane.GetDistanceToPoint(self->noteTransform->get_position());

    GameNoteController_HandleBigWasCutBySaber(self, saber, cutPoint, orientation, cutDirVec);
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GlobalNamespace::GameplayCoreSceneSetupData* self, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap, GlobalNamespace::IPreviewBeatmapLevel* previewBeatmapLevel, GlobalNamespace::GameplayModifiers* gameplayModifiers, GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings, GlobalNamespace::PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, GlobalNamespace::EnvironmentInfoSO* environmentInfo, GlobalNamespace::ColorScheme* colorScheme)
{
    leftColor = colorScheme->get_saberAColor();
    rightColor = colorScheme->get_saberBColor();
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme);
}

extern "C" void setup(ModInfo& info) {
    info.id = "SliceVisualizer";
    info.version = "4.2.0";
    modInfo = info;
    
    getConfig().Load();
    SaveConfig();
}

extern "C" void load() {
    QuestUI::Init();

    custom_types::Register::AutoRegister();
    QuestUI::Register::RegisterModSettingsViewController<SliceVisualizer::UIController*>(modInfo, "Slice Visualizer");

    // log("Installing hooks...");
    INSTALL_HOOK(logger().get(), SongUpdate);
    INSTALL_HOOK(logger().get(), SongStart);
    INSTALL_HOOK(logger().get(), NoteController_SendNoteWasCutEvent);
    INSTALL_HOOK(logger().get(), GameNoteController_HandleBigWasCutBySaber);
    INSTALL_HOOK(logger().get(), GameplayCoreSceneSetupData_ctor);
    // log("Installed all hooks!");
}
#include "../include/UI.hpp"
#include "../include/codegen.hpp"
using namespace QuestUI;
using namespace UnityEngine::UI;
using namespace UnityEngine;

#define CreateIncrement(parent, floatConfigValue, name, decimals, increment, hasMin, hasMax, minValue, maxValue) QuestUI::BeatSaberUI::CreateIncrementSetting(parent, name, decimals, increment, floatConfigValue.GetFloat(), hasMin, hasMax, minValue, maxValue, UnityEngine::Vector2{}, il2cpp_utils::MakeDelegate<UnityEngine::Events::UnityAction_1<float>*>(classof(UnityEngine::Events::UnityAction_1<float>*), (void*)nullptr, +[](float value) { floatConfigValue.SetFloat(value); }))

#define CreateToggle(parent, boolConfigValue, name) QuestUI::BeatSaberUI::CreateToggle(parent, name, boolConfigValue.GetBool(), il2cpp_utils::MakeDelegate<UnityEngine::Events::UnityAction_1<bool>*>(classof(UnityEngine::Events::UnityAction_1<bool>*), (void*)nullptr, +[](bool value) { boolConfigValue.SetBool(value); }))

DEFINE_CLASS(SliceVisualizer::UIController);

void SliceVisualizer::UIController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
	if(firstActivation)
	{
		VerticalLayoutGroup* layout = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(get_rectTransform());
		layout->set_spacing(1.5f);
        
        float spacing = 1.5f;
		
		VerticalLayoutGroup* layout1 = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(layout->get_rectTransform());
		layout1->set_spacing(spacing);
		layout1->get_gameObject()->AddComponent<QuestUI::Backgroundable*>()->ApplyBackground(il2cpp_utils::createcsstr("round-rect-panel"));

        ContentSizeFitter* contentSizeFitter = layout1->get_gameObject()->AddComponent<ContentSizeFitter*>();
		contentSizeFitter->set_horizontalFit(ContentSizeFitter::FitMode::PreferredSize);
		contentSizeFitter->set_verticalFit(ContentSizeFitter::FitMode::PreferredSize);

		layout1->set_padding(UnityEngine::RectOffset::New_ctor(3, 3, 2, 2));

		Transform* Parent1 = layout1->get_transform();

        CreateToggle(Parent1, getConfig().config["Enabled"], "Enabled");
        QuestUI::BeatSaberUI::AddHoverHint(CreateToggle(Parent1, getConfig().config["DynamicFadeSpeed"], "Dynamic Fade Speed")->get_gameObject(), "When enabled, slice visuals will fade faster during faster parts of songs");
        QuestUI::BeatSaberUI::AddHoverHint(CreateIncrement(Parent1, getConfig().config["FadeSpeed"], "Fade Speed", 0, 1, true, false, 0, 0.0f)->get_gameObject(), "The speed that slice visuals fade at, a higher value means it fades quicker");
    }
}

void SliceVisualizer::UIController::DidDeactivate(bool removedFromHierarchy, bool systemScreenDisabling)  {
    getConfig().Write();
}
#include "../src/game/GameFlow.h"
#include "../src/game/GameTuning.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace
{
    void Require(bool _condition, const char* _message)
    {
        if (!_condition)
        {
            throw std::runtime_error(_message);
        }
    }
}

int main()
{
    try
    {
        GameFlow flow;
        Require(flow.GetState() == ScreenState::MainMenu, "must start in main menu");
        Require(flow.GetSession() == nullptr, "main menu must not start a session");

        flow.StartGame();
        Require(flow.GetState() == ScreenState::FadingOut, "start should fade out");
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(flow.GetState() == ScreenState::FadingIn, "start should fade in");
        Require(flow.GetSession() != nullptr, "new game should be created during fade");
        Require(flow.GetVisibleState() == ScreenState::Countdown,
            "fade-in must not reveal the maze");
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(flow.GetState() == ScreenState::Countdown, "fade should lead to countdown");
        Require(flow.GetCountdownNumber() == GameTuning::Transition::CountdownSeconds, "countdown should start at 3");

        const float initialTime = flow.GetSession()->GetRemainingTime();
        flow.TryMove({ 0, 1 }); // Ignore movement during countdown.
        flow.Update(static_cast<float>(GameTuning::Transition::CountdownSeconds - 1));
        Require(flow.GetCountdownNumber() == 1, "countdown should be 1 after 2 seconds");
        Require(flow.GetSession()->GetRemainingTime() == initialTime,
            "countdown must not consume game time");

        flow.Update(1.0f);
        Require(flow.GetState() == ScreenState::Go, "countdown should show GO");
        Require(flow.GetVisibleState() == ScreenState::Go,
            "GO should be rendered without showing the maze");
        flow.TryMove({ 0, 1 }); // Ignore movement during GO.
        flow.Update(GameTuning::Transition::GoSeconds);
        Require(flow.GetState() == ScreenState::Playing,
            "GO should lead to gameplay");
        Require(flow.GetSession()->GetRemainingTime() == initialTime,
            "countdown and GO must not consume game time");

        flow.Pause();
        Require(flow.GetState() == ScreenState::Paused, "ESC should pause");
        Require(flow.GetVisibleState() == ScreenState::Paused,
            "paused state must not request gameplay rendering");
        flow.TryMove({ 0, 1 }); // Ignore movement during pause.
        flow.Update(10.0f);
        Require(flow.GetSession()->GetRemainingTime() == initialTime,
            "paused game timer must not advance");

        flow.Resume();
        Require(flow.GetState() == ScreenState::Playing,
            "resume should not add a countdown");
        flow.Update(0.2f);
        Require(std::fabs(flow.GetSession()->GetRemainingTime()
            - (initialTime - 0.2f)) < 0.001f,
            "playing should resume the same clock");

        flow.Pause();
        flow.ReturnToMainMenu();
        Require(flow.GetState() == ScreenState::FadingOut, "return should fade out");
        Require(flow.GetVisibleState() == ScreenState::Paused,
            "pause-to-menu fade must keep maze hidden");
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(flow.GetSession() == nullptr, "return should dispose session");
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(flow.GetState() == ScreenState::MainMenu, "return should reach menu");

        flow.StartGame();
        flow.Update(GameTuning::Transition::FadeSeconds);
        flow.Update(GameTuning::Transition::FadeSeconds);
        flow.Update(static_cast<float>(GameTuning::Transition::CountdownSeconds));
        Require(flow.GetState() == ScreenState::Go, "restart must show GO");
        flow.Update(GameTuning::Transition::GoSeconds);
        Require(flow.GetState() == ScreenState::Playing, "restart must reach gameplay");
        Require(flow.GetSession()->GetStage().GetNumber() == 1,
            "restarting must reset stage");
        Require(flow.GetSession()->GetCompletedStages() == 0,
            "restarting must reset completed stages");
        Require(flow.GetSession()->GetRemainingTime() == initialTime,
            "restarting must reset remaining time");

        flow.Update(initialTime + 1.0f);
        Require(flow.GetState() == ScreenState::Result,
            "time expiration should show result");
        flow.Update(20.0f);
        Require(flow.GetState() == ScreenState::Result,
            "result must wait for the main menu button");
        flow.ReturnToMainMenu();
        flow.Update(GameTuning::Transition::FadeSeconds);
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(flow.GetState() == ScreenState::MainMenu,
            "result main menu action should return to menu");

        std::cout << "GameFlow tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "GameFlow test failed: " << error.what() << '\n';
        return 1;
    }
}

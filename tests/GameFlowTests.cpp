#include "../src/game/GameFlow.h"

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
        flow.Update(0.35f);
        Require(flow.GetState() == ScreenState::FadingIn, "start should fade in");
        Require(flow.GetSession() != nullptr, "new game should be created during fade");
        flow.Update(0.35f);
        Require(flow.GetState() == ScreenState::Countdown, "fade should lead to countdown");
        Require(flow.GetCountdownNumber() == 3, "countdown should start at 3");

        const float initialTime = flow.GetSession()->GetRemainingTime();
        flow.TryMove({ 0, 1 }); // Ignore movement during countdown.
        flow.Update(2.0f);
        Require(flow.GetCountdownNumber() == 1, "countdown should be 1 after 2 seconds");
        Require(flow.GetSession()->GetRemainingTime() == initialTime,
            "countdown must not consume game time");

        flow.Update(1.0f);
        Require(flow.GetState() == ScreenState::Playing, "countdown should begin play");
        Require(flow.GetSession()->GetRemainingTime() == initialTime,
            "game must begin with full time");

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
        flow.Update(0.35f);
        Require(flow.GetSession() == nullptr, "return should dispose session");
        flow.Update(0.35f);
        Require(flow.GetState() == ScreenState::MainMenu, "return should reach menu");

        flow.StartGame();
        flow.Update(0.35f);
        flow.Update(0.35f);
        flow.Update(3.0f);
        Require(flow.GetSession()->GetStage().GetNumber() == 1,
            "restarting must reset stage");
        Require(flow.GetSession()->GetScore() == 0, "restarting must reset score");
        Require(flow.GetSession()->GetRemainingTime() == initialTime,
            "restarting must reset remaining time");

        flow.Update(initialTime + 1.0f);
        Require(flow.GetState() == ScreenState::Result,
            "time expiration should show result");
        flow.Update(20.0f);
        Require(flow.GetState() == ScreenState::Result,
            "result must wait for the main menu button");
        flow.ReturnToMainMenu();
        flow.Update(0.35f);
        flow.Update(0.35f);
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

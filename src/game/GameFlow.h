#pragma once

#include "GameSession.h"
#include "GameTuning.h"

#include <memory>
#include <random>

enum class ScreenState
{
    MainMenu,
    FadingOut,
    FadingIn,
    Countdown,
    Go,
    Playing,
    Paused,
    Result
};

// Controls screens and transitions; GameSession retains gameplay rules.
class GameFlow
{
public:
    GameFlow();

    void Update(float _deltaTime);
    void StartGame();
    void Pause();
    void Resume();
    void ReturnToMainMenu();
    void TryMove(const CellPosition& _direction);
    bool TryUseItem(ActiveItemType _item,
        const CellPosition& _direction = { 0, 0 });

    ScreenState GetState() const noexcept;
    ScreenState GetVisibleState() const noexcept;
    const GameSession* GetSession() const noexcept;
    float GetFadeAlpha() const noexcept;
    int GetCountdownNumber() const noexcept;

private:
    enum class Destination
    {
        NewGame,
        MainMenu
    };

    static constexpr float FadeDuration = GameTuning::Transition::FadeSeconds;
    static constexpr float CountdownDuration = static_cast<float>(GameTuning::Transition::CountdownSeconds);
    static constexpr float GoDuration = GameTuning::Transition::GoSeconds;

    std::mt19937 m_random;
    std::unique_ptr<GameSession> m_session;
    ScreenState m_state = ScreenState::MainMenu;
    ScreenState m_transitionSource = ScreenState::MainMenu;
    Destination m_destination = Destination::NewGame;
    float m_fadeElapsed = 0.0f;
    float m_countdownRemaining = CountdownDuration;
    float m_goRemaining = GoDuration;
};

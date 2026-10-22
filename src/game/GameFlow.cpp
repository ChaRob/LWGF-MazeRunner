#include "GameFlow.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

GameFlow::GameFlow()
    : m_random(std::random_device{}())
{
}

void GameFlow::Update(float _deltaTime)
{
    if (!std::isfinite(_deltaTime) || _deltaTime < 0.0f)
    {
        throw std::invalid_argument("Delta time must be finite and nonnegative.");
    }

    switch (m_state)
    {
    case ScreenState::FadingOut:
        m_fadeElapsed += _deltaTime;
        if (m_fadeElapsed >= FadeDuration)
        {
            // Swap the underlying content only when the screen is fully black.
            if (m_destination == Destination::NewGame)
            {
                m_session = std::make_unique<GameSession>(m_random());
            }
            else
            {
                m_session.reset();
            }

            m_state = ScreenState::FadingIn;
            m_fadeElapsed = 0.0f;
        }
        break;

    case ScreenState::FadingIn:
        m_fadeElapsed += _deltaTime;
        if (m_fadeElapsed >= FadeDuration)
        {
            m_fadeElapsed = 0.0f;
            if (m_destination == Destination::NewGame)
            {
                m_countdownRemaining = CountdownDuration;
                m_state = ScreenState::Countdown;
            }
            else
            {
                m_state = ScreenState::MainMenu;
            }
        }
        break;

    case ScreenState::Countdown:
        m_countdownRemaining -= _deltaTime;
        if (m_countdownRemaining <= 0.0f)
        {
            m_countdownRemaining = 0.0f;
            m_state = ScreenState::Playing;
        }
        break;

    case ScreenState::Playing:
        if (m_session)
        {
            m_session->Update(_deltaTime);
            if (m_session->GetStatus() == GameStatus::GameOver)
            {
                m_state = ScreenState::Result;
            }
        }
        break;

    case ScreenState::MainMenu:
    case ScreenState::Paused:
    case ScreenState::Result:
        // No gameplay clock runs outside Playing.
        break;
    }
}

void GameFlow::StartGame()
{
    if (m_state != ScreenState::MainMenu)
    {
        return;
    }

    m_transitionSource = ScreenState::MainMenu;
    m_destination = Destination::NewGame;
    m_fadeElapsed = 0.0f;
    m_state = ScreenState::FadingOut;
}

void GameFlow::Pause()
{
    if (m_state == ScreenState::Playing)
    {
        m_state = ScreenState::Paused;
    }
}

void GameFlow::Resume()
{
    if (m_state == ScreenState::Paused)
    {
        m_state = ScreenState::Playing;
    }
}

void GameFlow::ReturnToMainMenu()
{
    if (m_state != ScreenState::Paused && m_state != ScreenState::Result)
    {
        return;
    }

    m_transitionSource = m_state;
    m_destination = Destination::MainMenu;
    m_fadeElapsed = 0.0f;
    m_state = ScreenState::FadingOut;
}

void GameFlow::TryMove(const CellPosition& _direction)
{
    if (m_state == ScreenState::Playing && m_session)
    {
        m_session->TryMove(_direction);
    }
}

ScreenState GameFlow::GetState() const noexcept
{
    return m_state;
}

ScreenState GameFlow::GetVisibleState() const noexcept
{
    if (m_state == ScreenState::FadingOut)
    {
        return m_transitionSource;
    }

    if (m_state == ScreenState::FadingIn)
    {
        return m_destination == Destination::NewGame
            ? ScreenState::Playing
            : ScreenState::MainMenu;
    }

    return m_state;
}

const GameSession* GameFlow::GetSession() const noexcept
{
    return m_session.get();
}

float GameFlow::GetFadeAlpha() const noexcept
{
    const float progress = std::clamp(m_fadeElapsed / FadeDuration, 0.0f, 1.0f);
    if (m_state == ScreenState::FadingOut)
    {
        return progress;
    }

    if (m_state == ScreenState::FadingIn)
    {
        return 1.0f - progress;
    }

    return 0.0f;
}

int GameFlow::GetCountdownNumber() const noexcept
{
    if (m_state != ScreenState::Countdown)
    {
        return 0;
    }

    return std::clamp(static_cast<int>(std::ceil(m_countdownRemaining)), 1, 3);
}

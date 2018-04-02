#ifndef APPLICATION_PROFILE_HPP_INCLUDED
#define APPLICATION_PROFILE_HPP_INCLUDED

#include <funserialisation/serialise.hpp>
#include <imgui/imgui.h>

struct application_profile : serialisable
{
    ///not persisted. purely for bookkeeping
    bool applied = false;

    std::string name;

    bool auto_lock_mouse = false;
    bool auto_borderless = false;
    float init_delay_s = 1.f;

    bool should_move_application = true;
    int application_x = 0;
    int application_y = 0;

    bool enabled = true;

    virtual void do_serialise(serialise& s, bool ser) override
    {
        s.handle_serialise(name, ser);

        s.handle_serialise(auto_lock_mouse, ser);
        s.handle_serialise(auto_borderless, ser);
        s.handle_serialise(init_delay_s, ser);

        s.handle_serialise(should_move_application, ser);
        s.handle_serialise(application_x, ser);
        s.handle_serialise(application_y, ser);

        s.handle_serialise(enabled, ser);
    }

    void draw_window_internals()
    {
        ImGui::Text(name.c_str());

        ImGui::Checkbox("Auto Confine Mouse", &auto_lock_mouse);

        if(ImGui::IsItemHovered())
            ImGui::SetTooltip("Locks the mouse cursor to the game");

        ImGui::Checkbox("Auto Borderless", &auto_borderless);

        if(ImGui::IsItemHovered())
            ImGui::SetTooltip("Sets a windowed game to be borderless windowed");

        ImGui::Checkbox("Move Window?", &should_move_application);

        if(ImGui::IsItemHovered())
            ImGui::SetTooltip("If enabled, moves the game's window to Start x and Start y");

        if(should_move_application)
        {
            ImGui::InputInt("Start x", &application_x, 1, 100);
            ImGui::InputInt("Start y", &application_y, 1, 100);
        }

        ImGui::Checkbox("Enabled", &enabled);

        if(ImGui::IsItemHovered())
            ImGui::SetTooltip("Enable the profile?");
    }
};

#endif // APPLICATION_PROFILE_HPP_INCLUDED

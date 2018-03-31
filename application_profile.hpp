#ifndef APPLICATION_PROFILE_HPP_INCLUDED
#define APPLICATION_PROFILE_HPP_INCLUDED

#include <funserialisation/serialise.hpp>

struct application_profile : serialisable
{
    std::string name;

    bool auto_lock_mouse = false;
    bool auto_borderless = false;
    float init_delay_s = 1.f;

    int application_x = 0;
    int application_y = 0;

    bool enabled = true;

    virtual void do_serialise(serialise& s, bool ser) override
    {
        s.handle_serialise(name, ser);

        s.handle_serialise(auto_lock_mouse, ser);
        s.handle_serialise(auto_borderless, ser);
        s.handle_serialise(init_delay_s, ser);

        s.handle_serialise(application_x, ser);
        s.handle_serialise(application_y, ser);

        s.handle_serialise(enabled, ser);
    }

    void draw_window_internals()
    {
        ImGui::Text(name.c_str());

        ImGui::Checkbox("Auto Confine mouse", &auto_lock_mouse);
        ImGui::Checkbox("Auto borderless", &auto_borderless);

        ImGui::InputInt("Start x", &application_x, 1, 100);
        ImGui::InputInt("Start y", &application_y, 1, 100);

        ImGui::Checkbox("Enabled", &enabled);
    }
};

#endif // APPLICATION_PROFILE_HPP_INCLUDED

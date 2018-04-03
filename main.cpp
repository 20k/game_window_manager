#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <bitset>
#include <iostream>
#include <functional>

#include <SFML/Graphics.hpp>
#include <imgui/imgui.h>
#include <imgui-sfml/imgui-SFML.h>

#include <funserialisation/serialise.hpp>
#include "application_profile.hpp"
#include "winapi_interop.hpp"
#include "process_manager.hpp"

int main()
{
    sf::RenderWindow window;
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    window.create(sf::VideoMode(600, 400),"Wowee", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);

    //ImGui::NewFrame();

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 2;
    style.WindowRounding = 2;
    style.ChildRounding = 2;

    process_manager process_manage;

    sf::Keyboard key;
    bool toggled_end_key = false;
    bool toggled_f9_key = false;

    sf::Clock ui_clock;
    sf::Clock refresh_clock;
    sf::Clock save_clock;
    sf::Clock apply_clock;
    sf::Clock frametime_clock;

    std::ifstream test_file("save_data.bin");

    if(test_file.good())
    {
        serialise ser;
        ser.load("save_data.bin");

        ser.handle_serialise_no_clear(process_manage, false);
    }

    bool focused = true;
    bool going = true;

    double frametime_s = 0;

    int last_desired_w = window.getSize().x;

    while(going)
    {
        sf::Event event;

        while(window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);

            if(event.type == sf::Event::Closed)
                going = false;

            if(event.type == sf::Event::Resized)
            {
                window.setSize({event.size.width, event.size.height});
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
            }

            if(event.type == sf::Event::GainedFocus)
            {
                focused = true;
                window.setFramerateLimit(60);
            }

            if(event.type == sf::Event::LostFocus)
            {
                focused = false;
                window.setFramerateLimit(5);
            }
        }

        if(last_desired_w > 100 && abs(last_desired_w - (int)window.getSize().x) > 10)
        {
            int dx = last_desired_w;
            int dy = window.getSize().y;

            window.setSize({(unsigned int)dx, (unsigned int)dy});
            window.setView(sf::View(sf::FloatRect(0, 0, dx, dy)));
        }

        ImGui::SFML::Update(window, ui_clock.restart());

        frametime_s = frametime_clock.restart().asMicroseconds() / 1000. / 1000.;

        if(refresh_clock.getElapsedTime().asSeconds() > 1)
        {
            refresh_clock.restart();

            //process_manage.refresh();
        }

        if(save_clock.getElapsedTime().asSeconds() > 1)
        {
            save_clock.restart();

            serialise ser;
            ser.handle_serialise(process_manage, true);

            ser.save("save_data.bin");
        }

        if(apply_clock.getElapsedTime().asSeconds() > 1)
        {
            double time_elapsed_s = apply_clock.getElapsedTime().asMicroseconds() / 1000. / 1000.;

            apply_clock.restart();

            process_manage.check_apply_profile_to_foreground_window(time_elapsed_s);
        }

        if(!key.isKeyPressed(sf::Keyboard::End))
            toggled_end_key = false;

        if(key.isKeyPressed(sf::Keyboard::End) && !toggled_end_key && process_manage.use_mouse_lock)
        {
            toggled_end_key = true;

            process_manage.refresh();
            process_manage.toggle_mouse_lock();
            process_manage.handle_mouse_lock();
        }

        ///TODO: Use a better system for this
        if(!key.isKeyPressed(sf::Keyboard::F9))
            toggled_f9_key = false;

        if(key.isKeyPressed(sf::Keyboard::F9) && !toggled_f9_key && process_manage.use_f9_refresh)
        {
            toggled_f9_key = true;

            process_manage.refresh();
        }

        int desired_w = 0;
        process_manage.draw_window(desired_w);
        last_desired_w = desired_w;

        if(process_manage.should_quit)
            going = false;

        ImGui::SFML::Render(window);

        window.display();
        window.clear();

        sf::sleep(sf::milliseconds(4));
    }

    serialise ssave;
    ssave.handle_serialise(process_manage, true);
    ssave.save("save_data.bin");

    return 0;
}

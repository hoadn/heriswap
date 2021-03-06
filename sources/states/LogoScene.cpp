/*
    This file is part of Heriswap.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

    Heriswap is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    Heriswap is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Heriswap.  If not, see <http://www.gnu.org/licenses/>.
*/



#include "base/StateMachine.h"
#include "Scenes.h"

#include "base/EntityManager.h"
#include "base/TouchInputManager.h"
#include "base/PlacementHelper.h"

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/SoundSystem.h"
#include "systems/AnchorSystem.h"

#include "HeriswapGame.h"
#include "Game_Private.h"
#include "DepthLayer.h"

class LogoScene : public StateHandler<Scene::Enum> {
    HeriswapGame* game;
    Entity logo, animLogo, logobg;

public:

    LogoScene(HeriswapGame* game) : StateHandler<Scene::Enum>("logo_scene") {
        this->game = game;
    }

    void setup(AssetAPI*) override {
        logo = theEntityManager.CreateEntityFromTemplate("logo/logo");
        logobg = theEntityManager.CreateEntityFromTemplate("logo/logo_bg");
        animLogo = theEntityManager.CreateEntityFromTemplate("logo/logo_anim");

        glm::vec2 offset = glm::vec2(-10 / 800.0, 83/869.0) * TRANSFORM(logo)->size;
        ANCHOR(animLogo)->position = TRANSFORM(logo)->position + offset;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    #define FADE 1
    void onPreEnter(Scene::Enum) override {
        RENDERING(logo)->show = true;
        RENDERING(logobg)->show = true;
        game->datas->faderHelper.start(Fading::In, FADE);
        // preload sound
        theSoundSystem.loadSoundFile("audio/son_monte.ogg");
    }

    bool updatePreEnter(Scene::Enum, float dt) override {
        return game->datas->faderHelper.update(dt);
    }

    float timeAccum;
    bool soundPlayed;
    void onEnter(Scene::Enum) override {
        timeAccum = 0;
        soundPlayed = false;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//
    Scene::Enum update(float dt) {
        if (timeAccum > 0.8 + 0.05 + 0.25 + 0.05) {
            return Scene::MainMenu;
        } else if (timeAccum > 0.8 + 0.05 + 0.25) {
            RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
        }
        else if (timeAccum > 0.8 + 0.05) {
            if (!soundPlayed) {
                SOUND(animLogo)->sound = theSoundSystem.loadSoundFile("audio/son_monte.ogg");
                soundPlayed = true;
            }
            RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo3_365_331");
        }
        else if (timeAccum > 0.8) {
            RENDERING(animLogo)->show = true;
        }

        timeAccum += dt;
        return Scene::Logo;
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- EXIT SECTION -----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onPreExit(Scene::Enum) {
        RENDERING(animLogo)->show = false;
        game->datas->faderHelper.start(Fading::OutIn, 4 * FADE);
        game->datas->faderHelper.registerFadingOutEntity(logo);
        game->datas->faderHelper.registerFadingOutEntity(logobg);
    }

    bool updatePreExit(Scene::Enum, float dt) {
        return game->datas->faderHelper.update(dt);
    }

    void onExit(Scene::Enum) {
        theEntityManager.DeleteEntity(logo);
        theEntityManager.DeleteEntity(logobg);
        theEntityManager.DeleteEntity(animLogo);

        theRenderingSystem.unloadAtlas("logo");

        game->datas->faderHelper.clearFadingEntities();
    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateLogoSceneHandler(HeriswapGame* game) {
        return new LogoScene(game);
    }
}

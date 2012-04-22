/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit http://www.ogre3d.org/tikiwiki/Hydrax

Copyright (C) 2009 Xavier Verguín González <xavierverguin@hotmail.com>
                                           <xavyiy@gmail.com>

That have bnn modified by Jose Luis Cercós Pita.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.

Contributors:
Jose Luis Cercós Pita <jlcercos@gmail.com>
--------------------------------------------------------------------------------
*/

// ----------------------------------------------------------------------------
// Correct eventual problems with newer versions of libboost
// ----------------------------------------------------------------------------
#ifndef __stdcall
    #define __stdcall __attribute__((stdcall))
#endif

// ----------------------------------------------------------------------------
// Include the main OGRE header files
// Ogre.h just expands to including lots of individual OGRE header files
// ----------------------------------------------------------------------------
#include <Ogre.h>
#include <OgreTextAreaOverlayElement.h>

// ----------------------------------------------------------------------------
// Include the OGRE example framework
// This includes the classes defined to make getting an OGRE application running
// a lot easier. It automatically sets up all the main objects and allows you to
// just override the bits you want to instead of writing it all from scratch.
// ----------------------------------------------------------------------------
#include <ExampleApplication.h>

// ----------------------------------------------------------------------------
// Include the Hydrax plugin headers
// Main base headers (Hydrax.h) and especific headers (Noise/Water modules)
// ----------------------------------------------------------------------------
#include "Hydrax/Hydrax.h"
#include "Hydrax/Noise/FFT/FFT.h"
#include "Hydrax/Noise/Perlin/Perlin.h"
#include "Hydrax/Noise/Real/Real.h"
#include "Hydrax/Modules/ProjectedGrid/ProjectedGrid.h"
#include "Hydrax/Modules/RadialGrid/RadialGrid.h"
#include "Hydrax/Modules/SimpleGrid/SimpleGrid.h"

// ----------------------------------------------------------------------------
// Include HydrOCL. An OpenCL accelerated Hydrax module.
// ----------------------------------------------------------------------------
#include "hydrocl.h"

#define _def_SkyBoxNum 3

// Hydrax pointer
Hydrax::Hydrax *mHydrax = 0;

Ogre::String mSkyBoxes[_def_SkyBoxNum] =
           {"Sky/ClubTropicana",
			"Sky/EarlyMorning",
		    "Sky/Clouds"};

Ogre::Vector3 mSunPosition[_def_SkyBoxNum] =
           {Ogre::Vector3(0,10000,0),
			Ogre::Vector3(0,10000,90000),
		    Ogre::Vector3(0,10000,0)};

Ogre::Vector3 mSunColor[_def_SkyBoxNum] =
           {Ogre::Vector3(1, 0.9, 0.6),
			Ogre::Vector3(1,0.6,0.4),
		    Ogre::Vector3(0.45,0.45,0.45)};

int mCurrentSkyBox = 0;

// Just to wshow skyboxes information
Ogre::TextAreaOverlayElement* mTextArea = 0;

// ----------------------------------------------------------------------------
// Define the application object
// This is derived from ExampleApplication which is the class OGRE provides to
// make it easier to set up OGRE without rewriting the same code all the time.
// You can override extra methods of ExampleApplication if you want to further
// specialise the setup routine, otherwise the only mandatory override is the
// 'createScene' method which is where you set up your own personal scene.
// ----------------------------------------------------------------------------

class ExampleHydraxDemoListener : public ExampleFrameListener
{
public:
    SceneManager *mSceneMgr;
    Real mKeyBuffer;

    ExampleHydraxDemoListener(RenderWindow* win, Camera* cam, SceneManager *sm)
            : ExampleFrameListener(win,cam)
            , mSceneMgr(sm)
            , mKeyBuffer(-1)
    {
    }

    bool frameStarted(const FrameEvent &e)
    {
		// Check camera height
		Ogre::RaySceneQuery *raySceneQuery =
			mSceneMgr->
			     createRayQuery(Ogre::Ray(mCamera->getPosition() + Ogre::Vector3(0,1000000,0),
				                Vector3::NEGATIVE_UNIT_Y));
		Ogre::RaySceneQueryResult& qryResult = raySceneQuery->execute();
        Ogre::RaySceneQueryResult::iterator i = qryResult.begin();
        if (i != qryResult.end() && i->worldFragment)
        {
			if (mCamera->getPosition().y < i->worldFragment->singleIntersection.y + 3)
			{
                mCamera->
				     setPosition(mCamera->getPosition().x,
                                 i->worldFragment->singleIntersection.y + 3,
                                 mCamera->getPosition().z);
			}
        }

		delete raySceneQuery;

		// Update Hydrax
        mHydrax->update(e.timeSinceLastFrame);

		// Check for skyboxes switch
        mKeyboard->capture();

        if (mKeyboard->isKeyDown(OIS::KC_M) && mKeyBuffer < 0)
        {
			mCurrentSkyBox++;

			if(mCurrentSkyBox > (_def_SkyBoxNum-1))
			{
			    mCurrentSkyBox = 0;
			}
			changeSkyBox();

			mKeyBuffer = 0.5f;
		}

		mKeyBuffer -= e.timeSinceLastFrame;

        return true;
    }

    void changeSkyBox()
    {
        // printf("\t%s\n", mSkyBoxes[mCurrentSkyBox]);
        // Change skybox
        mSceneMgr->setSkyBox(true, mSkyBoxes[mCurrentSkyBox], 15000, true);

        // Update Hydrax sun position and colour
        mHydrax->setSunPosition(mSunPosition[mCurrentSkyBox]);
        mHydrax->setSunColor(mSunColor[mCurrentSkyBox]);

        // Update light 0 light position and colour
        mSceneMgr->getLight("Light0")->setPosition(mSunPosition[mCurrentSkyBox]);
        mSceneMgr->getLight("Light0")->setSpecularColour(mSunColor[mCurrentSkyBox].x,mSunColor[mCurrentSkyBox].y,mSunColor[mCurrentSkyBox].z);

		// Log
        LogManager::getSingleton().logMessage("Skybox " + mSkyBoxes[mCurrentSkyBox] + " selected. ("+Ogre::StringConverter::toString(mCurrentSkyBox+1)+"/"+Ogre::StringConverter::toString(_def_SkyBoxNum)+")");
    }
};

/** Just to locate palmiers with a pseudo-random algoritm
 */
float seed_ = 801;
float rnd_(const float& min, const float& max)
{
	seed_ += Ogre::Math::PI*2.8574f + seed_*(0.3424f - 0.12434f + 0.452345f);
	if (seed_ > 1000000000) seed_ -= 1000000000;
	return ((max-min)*Ogre::Math::Abs(Ogre::Math::Sin(Ogre::Radian(seed_))) + min);
}

void createPalms(Ogre::SceneManager *mSceneMgr)
{
	const int NumberOfPalms = 12;

	Ogre::SceneNode* mPalmsSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();

	for (int k = 0; k < NumberOfPalms; k++)
	{
		Ogre::Vector3 RandomPos = Ogre::Vector3(rnd_(0.f,300.f),
			0.f,
			rnd_(0.f,300.f));

		Ogre::RaySceneQuery * raySceneQuery = mSceneMgr->
			createRayQuery(Ogre::Ray(RandomPos + Ogre::Vector3(0.f,1000000.f,0.f),
			Ogre::Vector3::NEGATIVE_UNIT_Y));

		Ogre::RaySceneQueryResult& qryResult = raySceneQuery->execute();
		Ogre::RaySceneQueryResult::iterator i = qryResult.begin();

		if (i != qryResult.end() && i->worldFragment)
		{
			if (i->worldFragment->singleIntersection.y>10.5f || i->worldFragment->singleIntersection.y<2.f)
			{
			    printf("INFO (createPalms): Discarding palm.\n");
			    printf("\tThe palm is not in a correct place of island.\n");
				k--;
				continue;
			}

			RandomPos.y = i->worldFragment->singleIntersection.y;
		}
		else
		{
            printf("INFO (createPalms): Discarding palm.\n");
            printf("\tThe palm is not in the island.\n");
			k--;
			continue;
		}

        printf("INFO (createPalms): Inserting palm.\n");
        printf("\t%d of %d palms inserted.\n", k+1, NumberOfPalms);

		Ogre::Entity *mPalmEnt = mSceneMgr->createEntity("Palm"+Ogre::StringConverter::toString(k), "Palm.mesh");
		Ogre::SceneNode *mPalmSN = mPalmsSceneNode->createChildSceneNode();

		mPalmSN->rotate(Ogre::Vector3(-1.f,0.f,rnd_(-0.3f,0.3f)), Ogre::Degree(90.f));
		mPalmSN->attachObject(mPalmEnt);
		Ogre::Real Scale = rnd_(5.f,7.5f);
		mPalmSN->scale(Scale,Scale,Scale);
		mPalmSN->setPosition(RandomPos);
	}
}

class SampleApp : public ExampleApplication
{
public:
    // Basic constructor
    SampleApp()
    {}

protected:
	void chooseSceneManager()
    {
        // Create the SceneManager, in this case a generic one
        mSceneMgr = mRoot->createSceneManager("TerrainSceneManager");
    }

    // Just override the mandatory create scene method
    void createScene(void)
    {
		// Set default ambient light
		mSceneMgr->setAmbientLight(ColourValue(1, 1, 1));

        // Create the SkyBox
        mSceneMgr->setSkyBox(true, mSkyBoxes[mCurrentSkyBox]);

		// Set some camera params
		mCamera->setNearClipDistance(0.1f);
        mCamera->setFarClipDistance(30000.f);
		mCamera->setPosition(31.3f,20.6f,152.4f);
		mCamera->lookAt(Vector3(150.f,0.f,150.f));
		// mCamera->setOrientation(Ogre::Quaternion(0.998f, -0.0121, -0.0608, -0.00074));

	    // Light
		Ogre::Light *mLight = mSceneMgr->createLight("Light0");
		mLight->setPosition(mSunPosition[mCurrentSkyBox]);
		mLight->setDiffuseColour(1, 1, 1);
		mLight->setSpecularColour(mSunColor[mCurrentSkyBox].x,
			                      mSunColor[mCurrentSkyBox].y,
								  mSunColor[mCurrentSkyBox].z);

		// Hydrax initialization code ---------------------------------------------
		// ------------------------------------------------------------------------

        // Create Hydrax object
		mHydrax = new Hydrax::Hydrax(mSceneMgr, mCamera, mWindow->getViewport(0));

		// Create our projected grid module
		Hydrax::Module::HydrOCL *mModule
			= new Hydrax::Module::HydrOCL(// Hydrax parent pointer
			                              mHydrax,
			                              // Base plane
			                              Ogre::Plane(Ogre::Vector3(0,1,0), Ogre::Vector3(0,0,0)),
			                              // Projected grid options
			                              Hydrax::Module::HydrOCL::Options(/*Generic one*/));
		/* Not implemented yet
        // Add some waves
        static_cast<Hydrax::Noise::Real*>(mModule->getNoise())->addWave(
                                                Ogre::Vector2(1.f,0.f),
                                                0.3f,
                                                10.f);
        static_cast<Hydrax::Noise::Real*>(mModule->getNoise())->addWave(
                                                Ogre::Vector2(0.85f,0.15f),
                                                0.15f,
                                                8.f);
        static_cast<Hydrax::Noise::Real*>(mModule->getNoise())->addWave(
                                                Ogre::Vector2(0.95f,0.1f),
                                                0.1f,
                                                7.f);
		*/

		// Set our module
		mHydrax->setModule(static_cast<Hydrax::Module::Module*>(mModule));

		// Load all parameters from config file
		// Remarks: The config file must be in Hydrax resource group.
		// All parameters can be set/updated directly by code(Like previous versions),
		// but due to the high number of customizable parameters, since 0.4 version, Hydrax allows save/load config files.
		mHydrax->loadCfg("HydrOCLDemo.hdx");

        // Create water
        mHydrax->create();

		// Hydrax initialization code end -----------------------------------------
		// ------------------------------------------------------------------------

		// Load island

		mSceneMgr->setWorldGeometry("Island.cfg");
		mHydrax->getMaterialManager()->addDepthTechnique(
			static_cast<Ogre::MaterialPtr>(Ogre::MaterialManager::getSingleton().getByName("Island"))
			->createTechnique());
		// Create palmiers
		createPalms(mSceneMgr);
    }

    virtual void createFrameListener(void)
    {
        mFrameListener= new ExampleHydraxDemoListener(mWindow, mCamera, mSceneMgr);
        mFrameListener->showDebugOverlay(true);
        mRoot->addFrameListener(mFrameListener);
    }
};

// ----------------------------------------------------------------------------
// Main function, just boots the application object
// ----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    // Create application object
    SampleApp app;

    try
    {
        app.go();
    }
    catch ( Ogre::Exception& e )
    {
        std::cerr << "An exception has occured: " << e.getFullDescription();
    }

    return 0;
}

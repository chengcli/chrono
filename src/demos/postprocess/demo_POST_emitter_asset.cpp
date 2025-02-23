// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Alessandro Tasora
// =============================================================================
//
// Demo code about using particle emitters
//
// =============================================================================

#include "chrono/assets/ChTexture.h"
#include "chrono/particlefactory/ChParticleEmitter.h"
#include "chrono/particlefactory/ChParticleRemover.h"
#include "chrono/physics/ChSystemNSC.h"

#include "chrono_irrlicht/ChVisualSystemIrrlicht.h"

#include "chrono_postprocess/ChPovRay.h"

#include "chrono_thirdparty/filesystem/path.h"

// Use the main namespaces of Chrono, and other chrono namespaces
using namespace chrono;
using namespace chrono::particlefactory;
using namespace chrono::irrlicht;
using namespace chrono::postprocess;

int main(int argc, char* argv[]) {
    std::cout << "Copyright (c) 2017 projectchrono.org\nChrono version: " << CHRONO_VERSION << std::endl;

    // Create a Chrono system and set the associated collision system
    ChSystemNSC sys;
    sys.SetCollisionSystemType(ChCollisionSystem::Type::BULLET);

    // Create the Irrlicht visualization system
    auto vis = chrono_types::make_shared<ChVisualSystemIrrlicht>();
    vis->SetWindowSize(800, 600);
    vis->SetWindowTitle("Particle emitter: creation from various distributions");
    vis->Initialize();
    vis->AddLogo();
    vis->AddSkyBox();
    vis->AddTypicalLights();
    vis->AddCamera(ChVector3d(0, 4, -6), ChVector3d(0, -2, 0));

    // Create an exporter to POVray
    ChPovRay pov_exporter = ChPovRay(&sys);

    // Important: set the path to the template:
    pov_exporter.SetTemplateFile(GetChronoDataFile("POVRay_chrono_template.pov"));

    // Set the path where it will save all .pov, .ini, .asset and .dat files
    pov_exporter.SetBasePath(GetChronoOutputPath() + "EMITTER_ASSET");

    pov_exporter.SetLight(VNULL, ChColor(0, 0, 0), false);
    pov_exporter.SetCustomPOVcommandsScript(
        " \
         camera { \
              angle    45 \
              location <3.0 , 2.5 ,-18.0> \
              right    x*image_width/image_height \
              look_at  <0 , -2, 0> \
              rotate   <0,-180*(clock),0> \
          } \
	     light_source {   \
              <6, 15, -6>  \
	          color rgb<1.2,1.2,1.2> \
              area_light <5, 0, 0>, <0, 0, 5>, 8, 8 \
              adaptive 1 \
              jitter\
            } \
         box \
            {  \
                <20, 16, 20>, <0, 16, 0> \
                texture{ pigment{color rgb<3,3,3> }}    \
                finish { ambient 1 } \
            } \
          ");

    // CREATE THE SYSTEM OBJECTS

    // Create the floor:
    auto floor_mat = chrono_types::make_shared<ChContactMaterialNSC>();

    auto floor_body = chrono_types::make_shared<ChBodyEasyBox>(20, 1, 20, 1000, true, true, floor_mat);
    floor_body->SetPos(ChVector3d(0, -5, 0));
    floor_body->SetFixed(true);
    floor_body->GetVisualShape(0)->SetColor(ChColor(0.0f, 1.0f, (float)ChRandom::Get()));

    auto floor_shape = chrono_types::make_shared<ChCollisionShapeBox>(floor_mat, 20, 1, 20);
    floor_body->AddCollisionShape(floor_shape);

    // Custom rendering in POVray:
    pov_exporter.SetCustomCommands(floor_body,
                                   "texture{ pigment{ color rgb<1,1,1>}} \n\
                             texture{ Raster(4, 0.02, rgb<0.8,0.8,0.8>) } \n\
                             texture{ Raster(4, 0.02, rgb<0.8,0.8,0.8>) rotate<0,90,0> } \n\
                             texture{ Raster(4*0.2, 0.04, rgb<0.8,0.8,0.8>) } \n\
                             texture{ Raster(4*0.2, 0.04, rgb<0.8,0.8,0.8>) rotate<0,90,0> } \n\
                              ");

    sys.Add(floor_body);

    int num_emitters = 5;
    std::vector<ChParticleEmitter> emitters(num_emitters);
    for (unsigned int ie = 0; ie < emitters.size(); ie++) {
        // Ok, that object will take care of generating particle flows for you.
        // It accepts a lot of settings, for creating many different types of particle
        // flows, like fountains, outlets of various shapes etc.
        // For instance, set the flow rate, etc:

        emitters[ie].ParticlesPerSecond() = 3000;

        emitters[ie].SetUseParticleReservoir(true);
        emitters[ie].ParticleReservoirAmount() = 4000;

        // ---Initialize the randomizer for positions
        double xpos = (ie - 0.5 * num_emitters) * 2.2;
        auto emitter_positions = chrono_types::make_shared<ChRandomParticlePositionRectangleOutlet>();
        emitter_positions->Outlet() =
            ChCoordsys<>(ChVector3d(xpos, -4, 0), QuatFromAngleX(CH_PI_2));  // center and alignment of the outlet
        emitter_positions->OutletWidth() = 1.2;
        emitter_positions->OutletHeight() = 1.2;
        emitters[ie].SetParticlePositioner(emitter_positions);

        // just for visualizing outlet
        auto boxbody = chrono_types::make_shared<ChBodyEasyBox>(1.2, 0.4, 1.2, 3000, true, false);
        boxbody->SetPos(ChVector3d(xpos, -4.1, 0));
        boxbody->SetFixed(true);
        boxbody->GetVisualShape(0)->SetColor(ChColor(1.0f, 0.5f, 0.1f));
        sys.Add(boxbody);

        // ---Initialize the randomizer for alignments
        auto emitter_rotations = chrono_types::make_shared<ChRandomParticleAlignmentUniform>();
        emitters[ie].SetParticleAligner(emitter_rotations);

        // ---Initialize the randomizer for velocities, with statistical distribution
        auto mvelo = chrono_types::make_shared<ChRandomParticleVelocityConstantDirection>();
        mvelo->SetDirection(VECT_Y);
        mvelo->SetModulusDistribution(8.0);

        emitters[ie].SetParticleVelocity(mvelo);

        // A)
        // Create a ChRandomShapeCreator object (ex. here for sphere particles)

        auto creator_spheres = chrono_types::make_shared<ChRandomShapeCreatorSpheres>();
        creator_spheres->SetDiameterDistribution(chrono_types::make_shared<ChUniformDistribution>(0.06, 0.20));
        creator_spheres->SetDensityDistribution(chrono_types::make_shared<ChConstantDistribution>(1600));

        // Optional: define a callback to be exectuted at each creation of a sphere particle:
        class MyCreator_spheres : public ChRandomShapeCreator::AddBodyCallback {
            // Here do custom stuff on the just-created particle:
          public:
            virtual void OnAddBody(std::shared_ptr<ChBody> body,
                                   ChCoordsys<> coords,
                                   ChRandomShapeCreator& creator) override {
                body->GetVisualShape(0)->SetColor(ChColor(0.4f, 0.4f, 0.4f));
                pov->SetCustomCommands(body, " texture {finish { specular 0.9 } pigment{ color rgb<0.8,0.5,0.3>} }\n");

                // Bind the collision model to the collision system
                if (body->GetCollisionModel())
                    coll->Add(body->GetCollisionModel());
            }

            ChPovRay* pov;
            ChCollisionSystem* coll;
        };

        auto callback_spheres = chrono_types::make_shared<MyCreator_spheres>();
        callback_spheres->pov = &pov_exporter;
        callback_spheres->coll = sys.GetCollisionSystem().get();
        creator_spheres->RegisterAddBodyCallback(callback_spheres);

        // B)
        // Create a ChRandomShapeCreator object (ex. here for hull particles)

        auto creator_hulls = chrono_types::make_shared<ChRandomShapeCreatorConvexHulls>();
        creator_hulls->SetChordDistribution(chrono_types::make_shared<ChUniformDistribution>(0.15, 0.68));
        creator_hulls->SetDensityDistribution(chrono_types::make_shared<ChConstantDistribution>(1600));

        // Optional: define a callback to be exectuted at each creation of a sphere particle:
        class MyCreator_hulls : public ChRandomShapeCreator::AddBodyCallback {
            // Here do custom stuff on the just-created particle:
          public:
            virtual void OnAddBody(std::shared_ptr<ChBody> body,
                                   ChCoordsys<> coords,
                                   ChRandomShapeCreator& creator) override {
                body->GetVisualShape(0)->SetColor(ChColor(0.4f, 0.4f, 0.4f));
                pov->SetCustomCommands(body, " texture {finish { specular 0.9 } pigment{ color rgb<0.3,0.4,0.6>} }\n");

                // Bind the collision model to the collision system
                if (body->GetCollisionModel())
                    coll->Add(body->GetCollisionModel());
            }

            ChPovRay* pov;
            ChCollisionSystem* coll;
        };

        auto callback_hulls = chrono_types::make_shared<MyCreator_hulls>();
        callback_hulls->pov = &pov_exporter;
        callback_hulls->coll = sys.GetCollisionSystem().get();
        creator_hulls->RegisterAddBodyCallback(callback_hulls);

        // Create a parent ChRandomShapeCreator that 'mixes' some generators above, mixing them with given percents
        auto mcreatorTot = chrono_types::make_shared<ChRandomShapeCreatorFromFamilies>();
        mcreatorTot->AddFamily(creator_spheres, (double)ie / (double)(num_emitters - 1));
        mcreatorTot->AddFamily(creator_hulls, 1.0 - (double)ie / (double)(num_emitters - 1));
        mcreatorTot->Setup();

        // Finally, tell to the emitter that it must use the 'mixer' above:
        emitters[ie].SetParticleCreator(mcreatorTot);

        // --- Optional: what to do by default on ALL newly created particles?
        //     A callback executed at each particle creation can be attached to the emitter.
        //     For example, we need that new particles will be bound to the visualization and collision systems.

        // a- define a class that implement your custom OnAddBody method...
        class MyCreatorForAll : public ChRandomShapeCreator::AddBodyCallback {
          public:
            virtual void OnAddBody(std::shared_ptr<ChBody> body,
                                   ChCoordsys<> coords,
                                   ChRandomShapeCreator& creator) override {
                // Bind the visual model to the visualization system
                vis->BindItem(body);

                // Bind the collision model to the collision system
                if (body->GetCollisionModel())
                    coll->Add(body->GetCollisionModel());

                // Enable PovRay rendering
                pov->Add(body);

                // Disable gyroscopic forces for increased integrator stabilty
                body->SetUseGyroTorque(false);
            }
            ChVisualSystem* vis;
            ChPovRay* pov;
            ChCollisionSystem* coll;
        };

        // b- create the callback object...
        auto mcreation_callback = chrono_types::make_shared<MyCreatorForAll>();
        // c- set callback own data that he might need...
        mcreation_callback->vis = vis.get();
        mcreation_callback->coll = sys.GetCollisionSystem().get();
        mcreation_callback->pov = &pov_exporter;
        // d- attach the callback to the emitter!
        emitters[ie].RegisterAddBodyCallback(mcreation_callback);
    }

    // Bind all existing visual shapes to the visualization system
    vis->AttachSystem(&sys);

    // Export all existing visual shapes to POV-Ray
    pov_exporter.AddAll();

    // Create the .pov and .ini files for POV-Ray (this must be done only once at the beginning of the simulation)
    pov_exporter.ExportScript();

    // Simulation loop
    double timestep = 0.01;
    while (vis->Run()) {
        vis->BeginScene();
        vis->Render();
        vis->EndScene();

        // Continuosly create particle flow
        for (unsigned int ie = 0; ie < emitters.size(); ie++) {
            double tstart = ((double)ie / (double)num_emitters) * 1;
            double tend = tstart + 0.3;
            ChFunctionPoly23 mfuns(3000, tstart, tend);
            emitters[ie].ParticlesPerSecond() = mfuns.GetVal(sys.GetChTime());
            emitters[ie].EmitParticles(sys, timestep);
            // std::cout << ie << "  " << tstart << " " << mfuns.GetVal(application.GetSystem()->GetChTime()) << " " <<
            // emitters[ie].ParticlesPerSecond() << std::endl;
        }

        sys.DoStepDynamics(timestep);

        // Create the incremental nnnn.dat and nnnn.pov files that will be load
        // by the pov .ini script in POV-Ray (do this at each simulation timestep)
        pov_exporter.ExportData();
    }

    return 0;
}

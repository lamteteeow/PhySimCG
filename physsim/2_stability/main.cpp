#include "physsim_window.hpp"
#include "simulation.hpp"

#include <imgui.h>
#include <vislab/graphics/const_texture.hpp>
#include <vislab/graphics/diffuse_bsdf.hpp>
#include <vislab/graphics/perspective_camera.hpp>
#include <vislab/graphics/point_light.hpp>
#include <vislab/graphics/rectangle.hpp>
#include <vislab/graphics/scene.hpp>
#include <vislab/graphics/sphere.hpp>

using namespace vislab;

namespace physsim
{
    /**
     * @brief Stores parameters of a spring.
     */
    struct Spring
    {
        /**
         * @brief Stiffness constant.
         */
        double stiffness;

        /**
         * @brief Rest length of the spring.
         */
        double length;

        /**
         * @brief Damping factor.
         */
        double damping;

        /**
         * @brief Mass of the end point.
         */
        double mass;

        /**
         * @brief (Fixed) start point of the spring.
         */
        Eigen::Vector3d startPosition;

        /**
         * @brief End point of the spring.
         */
        Eigen::Vector3d endPosition;

        /**
         * @brief Velocity of end point of the spring.
         */
        Eigen::Vector3d endVelocity;
    };

    /**
     * @brief Stability of numerical integrators.
     */
    class StabilitySimulation : public Simulation
    {
    public:
        /**
         * @brief Enumeration of integration methods.
         */
        enum EMethod
        {
            Analytical,
            ExplicitEuler,
            SymplecticEuler,
            ExplicitRK2,
            ImplicitEuler
        };

        /**
         * @brief Initializes the scene.
         */
        void init() override
        {
            // initial simulation parameters
            mStepSize             = 1E-2;
            mGravity              = Eigen::Vector3d(0, 0, -9.8065);
            mSpring.mass          = 1.;
            mSpring.length        = 5.;
            mSpring.damping       = 0.1;
            mSpring.stiffness     = 5.;
            mSpring.startPosition = Eigen::Vector3d(0, 0, 0);
            mSpring.endPosition   = mSpring.startPosition + Eigen::Vector3d(0, 0, -mSpring.length);
            mSpring.endVelocity   = Eigen::Vector3d(0, 0, 0);

            // create a sphere for the end point and assign a bsdf
            auto sphere  = std::make_shared<Sphere>();
            sphere->bsdf = std::make_shared<DiffuseBSDF>(std::make_shared<ConstTexture>(Spectrum(1, 0, 0)));
            sphere->transform.setMatrix(Eigen::Matrix4d::translate(mSpring.endPosition));

            // create a point light
            auto light       = std::make_shared<PointLight>();
            light->intensity = Eigen::Vector3d(100.0, 100.0, 100.0);
            light->transform.setMatrix(Eigen::Matrix4d::translate(Eigen::Vector3d(-5, -3, -5)));

            // create a camera
            auto camera = std::make_shared<PerspectiveCamera>();
            camera->setLookAt(Eigen::Vector3d(0, 0, -7));
            camera->setPosition(Eigen::Vector3d(-10, 0, -7));
            camera->setUp(Eigen::Vector3d(0, 0, 1));
            camera->setNear(0.01);
            camera->setFar(100);

            // add elements to scene
            scene->camera = camera;
            scene->shapes.push_back(sphere);
            scene->lights.push_back(light);
        }

        /**
         * @brief Restarts the simulation.
         */
        void restart() override
        {
            // reset the simulation
            mTime               = 0;
            mSpring.endPosition = mSpring.startPosition + Eigen::Vector3d(0, 0, -mSpring.length);
            mSpring.endVelocity = Eigen::Vector3d::Zero();
            scene->shapes[0]->transform.setMatrix(Eigen::Matrix4d::translate(mSpring.endPosition));
        }

        /**
         * @brief Advances the simulation one time step forward.
         * @param elapsedTime Elapsed time in milliseconds during the last frame.
         * @param totalTime Total time in milliseconds since the beginning of the first frame.
         * @param timeStep Time step of the simulation. Restarts when resetting the simulation.
         */
        void advance(double elapsedTime, double totalTime, int64_t timeStep) override
        {
            // grab parameters
            Eigen::Vector3d spring_dir = (mSpring.endPosition - mSpring.startPosition).normalized();
            double spring_norm         = (mSpring.endPosition - mSpring.startPosition).norm();
            double k                   = mSpring.stiffness;
            double gamma               = mSpring.damping;
            double m                   = mSpring.mass;
            double L                   = mSpring.length;

            double alpha = -gamma / (2 * m);
            double beta  = sqrt(4 * k * m - pow(gamma, 2) / (2 * m));
            double c1    = -m * mGravity[2] / k;
            double c2    = -c1 * alpha / beta;

            // compute force
            Eigen::Vector3d f_int  = -k * (spring_norm - L) * spring_dir;
            Eigen::Vector3d f_damp = -gamma * mSpring.endVelocity;
            Eigen::Vector3d f_ext  = m * mGravity;
            Eigen::Vector3d f      = f_int + f_damp + f_ext;
            // a = f / m
            Eigen::Vector3d a = f / m;
            Eigen::Vector3d v = mSpring.endVelocity; // previous velocity
            Eigen::Vector3d x = mSpring.endPosition; // previous position

            // note that it is required to update both m_spring.end and p_cube's position
            switch (mMethod)
            {
            case EMethod::Analytical:
            {
                // TODO: analytical solution
                double z            = c1 * exp(alpha * mTime) * cos(beta * mTime) + c2 * exp(alpha * mTime) * sin(beta * mTime) - L + m * mGravity[2] / k;
                double w            = exp(alpha * mTime) * (c1 * (alpha * cos(beta * mTime) - beta * sin(beta * mTime)) + c2 * (alpha * sin(beta * mTime) + beta * cos(beta * mTime)));
                mSpring.endPosition = mSpring.startPosition + Eigen::Vector3d(0, 0, z);
                mSpring.endVelocity = Eigen::Vector3d(0, 0, w);
            }

            case EMethod::ExplicitEuler:
                // TODO: explicit euler
                mSpring.endVelocity += mStepSize * a;   
                mSpring.endPosition += mStepSize * v;

            case EMethod::SymplecticEuler:
                // TODO: symplectic euler
                mSpring.endVelocity += mStepSize * a;
                mSpring.endPosition += mStepSize * mSpring.endVelocity;

            case EMethod::ExplicitRK2:
            {
                // TODO: explicit second-order Runge-Kutta
                /*Eigen::Vector3d k1 = v;
                Eigen::Vector3d k2 = v + mStepSize / 2 * a;
                mSpring.endPosition += mStepSize * k2;
                mSpring.endVelocity += mStepSize * a;*/
                mSpring.endVelocity += mStepSize / 2 * a;
                mSpring.endPosition += mStepSize * mSpring.endVelocity;
                mSpring.endVelocity += mStepSize / 2 * a;
            }

            case EMethod::ImplicitEuler:
            {
                // TODO: implicit euler
                double w = (m * v[2] - mStepSize * k * x[2] + mStepSize * mGravity[2] * m - mStepSize * L * k) / (k * pow(mStepSize, 2) + gamma * mStepSize + m);
                mSpring.endVelocity = Eigen::Vector3d(0, 0, w);
                mSpring.endPosition += mStepSize * mSpring.endVelocity;
            }
            }

            // update spring end position
            scene->shapes[0]->transform.setMatrix(Eigen::Matrix4d::translate(mSpring.endPosition));

            mTime += mStepSize;
        }

        /**
         * @brief Adds graphical user interface elements with imgui.
         */
        void gui() override
        {
            ImGui::PushItemWidth(100);

            ImGui::Combo("method", (int*)&mMethod, "analytical\0explicit euler\0symplectic euler\0explicit RK2\0implicit euler\0\0");

            double stepSizeMin = 1E-3, stepSizeMax = 1E-1;
            ImGui::SliderScalar("dt", ImGuiDataType_Double, &mStepSize, &stepSizeMin, &stepSizeMax);

            double dampingMin = 0, dampingMax = 5E-1;
            ImGui::SliderScalar("damping", ImGuiDataType_Double, &mSpring.damping, &dampingMin, &dampingMax);

            double stiffnessMin = 0, stiffnessMax = 10;
            ImGui::SliderScalar("stiffness", ImGuiDataType_Double, &mSpring.stiffness, &stiffnessMin, &stiffnessMax);

            ImGui::PopItemWidth();
        }

    private:
        /**
         * @brief
         */
        EMethod mMethod;

        /**
         * @brief Position of the body.
         */
        Eigen::Vector3d mPosition;

        /**
         * @brief Linear velocity of the body.
         */
        Eigen::Vector3d mVelocity;

        /**
         * @brief Simulation time.
         */
        double mTime;

        /**
         * @brief Integration step size.
         */
        double mStepSize;

        /**
         * @brief Gravity vector.
         */
        Eigen::Vector3d mGravity;

        /**
         * @brief State of the spring.
         */
        Spring mSpring;
    };
}

int main()
{
    physsim::PhyssimWindow window(
        800,           // width
        600,           // height
        "2_stability", // title
        std::make_shared<physsim::StabilitySimulation>(),
        false // fullscreen
    );

    return window.run();
}

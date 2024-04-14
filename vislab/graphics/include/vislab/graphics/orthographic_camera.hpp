#pragma once

#include "projective_camera.hpp"

namespace vislab
{
    /**
     * @brief Class that implements a projective camera.
     */
    class OrthographicCamera : public ProjectiveCamera
    {
        VISLAB_OBJECT(OrthographicCamera, ProjectiveCamera)

    public:
        /**
         * @brief Constructor.
         */
        explicit OrthographicCamera();

    private:
        /**
         * @brief Recomputes the projection matrix.
         */
        void updateProjMatrix() override;
    };
}

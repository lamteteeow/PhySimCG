#pragma once

#include "base_points_fwd.hpp"

#include "ipoints.hpp"

#include <vislab/core/base_array.hpp>

namespace vislab
{
    /**
     * @brief Base class for point geometry with a certain dimensionality.
     * @tparam TD Dummy parameter needed to make this type reflection compatible.
     * @tparam TDimensions Number of dimensions.
     */
    template <int64_t TDimensions>
    class BasePoints : public IPoints
    {
        VISLAB_INTERFACE(BasePoints, IPoints)

    public:
        /**
         * @brief Number of spatial dimensions.
         */
        static const int64_t Dimensions = TDimensions;

        /**
         * @brief Bounding box type for this geometry.
         */
        using BoundingBox = Eigen::AlignedBox<double, TDimensions>;

        /**
         * @brief Constructor.
         */
        BasePoints()
        {
            mBoundingBox.setEmpty();
        }

        /**
         * @brief Copy-constructor.
         * @param other Geometry to copy from.
         */
        BasePoints(const BasePoints& other)
            : IPoints(other)
        {
            mBoundingBox = other.mBoundingBox;
        }

        /**
         * @brief Gets the vertex position array.
         * @return Vertex position array.
         */
        std::shared_ptr<BaseArray<TDimensions>> getVertices() { return std::static_pointer_cast<BaseArray<TDimensions>>(getVerticesImpl()); }

        /**
         * @brief Gets the vertex position array.
         * @return Vertex position array.
         */
        std::shared_ptr<const BaseArray<TDimensions>> getVertices() const { return std::static_pointer_cast<const BaseArray<TDimensions>>(getVerticesImpl()); }

        /**
         * @brief Gets the bounding box of the vertices. Note that recomputeBoundingBox has to be called first!
         * @return Bounding box of this geometry.
         */
        const BoundingBox& getBoundingBox() const { return mBoundingBox; }

        /**
         * @brief Tests if two point geometries are equal.
         * @param other Points to compare to.
         * @return True, if the geometries are equal.
         */
        bool isEqual(const IPoints* other) const override
        {
            if (!IPoints::isEqual(other))
                return false;
            const BasePoints* otherTyped = dynamic_cast<const BasePoints*>(other);
            if (otherTyped == nullptr)
                return false;
            if (!mBoundingBox.isApprox(otherTyped->mBoundingBox))
                return false;
            return true;
        }

        /**
         * @brief Removes all vertices and attributes.
         */
        void clear() override
        {
            IPoints::clear();
            mBoundingBox.setEmpty();
        }

    protected:
        /**
         * @brief Bounding box of the vertices.
         */
        BoundingBox mBoundingBox;
    };
}

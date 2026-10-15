#pragma once

#ifndef MOCK_TESTING
#include "../StdAfx.h"
#include "../StateManager.h"
#include <d3dx9.h>
#endif
#include <optional>

namespace EterLib::Render
{
    /**
     * @class WorldMatrixScope
     * @brief A modern C++23 RAII scope guard for managing the D3DTS_WORLD matrix.
     *
     * In the context of Metin2's modernization efforts (2026), managing the Direct3D
     * transformation state securely and reliably is essential to prevent rendering
     * anomalies like misplaced bones, models, or attached weapons.
     *
     * WorldMatrixScope takes a snapshot of the current D3DTS_WORLD matrix via CStateManager,
     * allowing subsequent code to modify the state safely. When the scope object is
     * destroyed, the saved matrix is automatically restored.
     *
     * It also supports immediate multiplication with a provided matrix, which is highly
     * useful for rendering hierarchical structures (like character bones and attached
     * weapons) where a child's transform is multiplied by the parent's current world transform.
     */
    class WorldMatrixScope final
    {
    public:
        /**
         * @brief Constructs a new World Matrix Scope.
         *
         * Retrieves the current D3DTS_WORLD matrix from STATEMANAGER.
         * If applyMatrix is provided, it multiplies the current world matrix by
         * applyMatrix and sets it as the new active D3DTS_WORLD matrix.
         * Otherwise, it just saves the current state for later restoration.
         *
         * @param applyMatrix Optional matrix to multiply with the current world state.
         */
        explicit WorldMatrixScope(const D3DXMATRIX* applyMatrix = nullptr)
        {
            InitializeScope(applyMatrix);
        }

        /**
         * @brief Destructor that restores the original D3DTS_WORLD matrix.
         *
         * Using RAII ensures that even in the presence of early returns, breaks,
         * or exceptions (though rare in this codebase), the rendering pipeline's
         * state is never left corrupted.
         */
        ~WorldMatrixScope()
        {
            RestoreScope();
        }

        // Disable copy and move semantics to prevent double-restoration or scope corruption.
        WorldMatrixScope(const WorldMatrixScope&) = delete;
        WorldMatrixScope& operator=(const WorldMatrixScope&) = delete;
        WorldMatrixScope(WorldMatrixScope&&) = delete;
        WorldMatrixScope& operator=(WorldMatrixScope&&) = delete;

        /**
         * @brief Multiplies the current D3DTS_WORLD matrix by the provided matrix.
         *
         * This updates the active transform without changing the saved state that
         * will be restored when the scope ends.
         *
         * @param matrix The matrix to apply.
         */
        void ApplyMatrix(const D3DXMATRIX& matrix)
        {
            D3DXMATRIX currentWorld;
            STATEMANAGER.GetTransform(D3DTS_WORLD, &currentWorld);

            D3DXMATRIX newWorld;
            D3DXMatrixMultiply(&newWorld, &matrix, &currentWorld);

            // Since we are already inside the scope, we do not call SaveTransform
            // again which would push another state to the stack. We directly
            // SetTransform.
            STATEMANAGER.SetTransform(D3DTS_WORLD, &newWorld);
        }

        /**
         * @brief Applies a translation transformation to the current world matrix.
         *
         * @param x The translation along the X axis.
         * @param y The translation along the Y axis.
         * @param z The translation along the Z axis.
         */
        void ApplyTranslation(float x, float y, float z)
        {
            D3DXMATRIX translation;
            D3DXMatrixTranslation(&translation, x, y, z);
            ApplyMatrix(translation);
        }

        /**
         * @brief Applies a rotation transformation to the current world matrix.
         *
         * @param yaw The rotation around the Y axis (in radians).
         * @param pitch The rotation around the X axis (in radians).
         * @param roll The rotation around the Z axis (in radians).
         */
        void ApplyRotation(float yaw, float pitch, float roll)
        {
            D3DXMATRIX rotation;
            D3DXMatrixRotationYawPitchRoll(&rotation, yaw, pitch, roll);
            ApplyMatrix(rotation);
        }

        /**
         * @brief Applies a scaling transformation to the current world matrix.
         *
         * @param x The scale factor along the X axis.
         * @param y The scale factor along the Y axis.
         * @param z The scale factor along the Z axis.
         */
        void ApplyScale(float x, float y, float z)
        {
            D3DXMATRIX scaling;
            D3DXMatrixScaling(&scaling, x, y, z);
            ApplyMatrix(scaling);
        }

        /**
         * @brief Resets the active D3DTS_WORLD matrix to the state it had when
         * this scope was created.
         *
         * Useful if multiple sibling elements need to be rendered in the same
         * base scope without interfering with each other's local transforms.
         */
        void ResetToBaseState()
        {
            STATEMANAGER.SetTransform(D3DTS_WORLD, &m_originalMatrix);
        }

        /**
         * @brief Retrieves the current active D3DTS_WORLD matrix.
         *
         * @return D3DXMATRIX The current world matrix.
         */
        [[nodiscard]] D3DXMATRIX GetCurrentMatrix() const
        {
            D3DXMATRIX currentWorld;
            STATEMANAGER.GetTransform(D3DTS_WORLD, &currentWorld);
            return currentWorld;
        }

        /**
         * @brief Retrieves the original D3DTS_WORLD matrix saved when the scope started.
         *
         * @return D3DXMATRIX The original world matrix.
         */
        [[nodiscard]] D3DXMATRIX GetOriginalMatrix() const
        {
            return m_originalMatrix;
        }

    private:
        /**
         * @brief Initializes the scope by saving the current state and optionally applying a matrix.
         *
         * @param applyMatrix The optional matrix to multiply.
         */
        void InitializeScope(const D3DXMATRIX* applyMatrix)
        {
            // Retrieve and store the original matrix for potential reset operations.
            STATEMANAGER.GetTransform(D3DTS_WORLD, &m_originalMatrix);

            if (applyMatrix)
            {
                D3DXMATRIX newWorld;
                // Correct multiplication order: child * parent
                D3DXMatrixMultiply(&newWorld, applyMatrix, &m_originalMatrix);

                // SaveTransform pushes the new matrix to the state stack and makes it active.
                STATEMANAGER.SaveTransform(D3DTS_WORLD, &newWorld);
            }
            else
            {
                // Save the current matrix to the state stack to be restored later.
                STATEMANAGER.SaveTransform(D3DTS_WORLD, &m_originalMatrix);
            }
        }

        /**
         * @brief Restores the scope by popping the state stack.
         */
        void RestoreScope()
        {
            STATEMANAGER.RestoreTransform(D3DTS_WORLD);
        }

        D3DXMATRIX m_originalMatrix;
    };
}

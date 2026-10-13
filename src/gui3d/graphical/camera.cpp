#include "camera.h"
#include <cmath>

const float CAMERA_SPEED = 0.2f;
const float MOUSE_SENSITIVITY = 0.005f;

void UpdateCameraWithKeyboard(Camera &camera)
{
    Vector3 forward = {camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z};
    float forwardLen = sqrtf(forward.x * forward.x + forward.z * forward.z);

    if (forwardLen > 0) {
        forward.x /= forwardLen;
        forward.z /= forwardLen;
    }

    Vector3 right = { forward.z, 0.0f, -forward.x };

    if (IsKeyDown(KEY_UP)) {
        camera.position.x += forward.x * CAMERA_SPEED;
        camera.position.z += forward.z * CAMERA_SPEED;
        camera.target.x += forward.x * CAMERA_SPEED;
        camera.target.z += forward.z * CAMERA_SPEED;
    }
    if (IsKeyDown(KEY_DOWN)) {
        camera.position.x -= forward.x * CAMERA_SPEED;
        camera.position.z -= forward.z * CAMERA_SPEED;
        camera.target.x -= forward.x * CAMERA_SPEED;
        camera.target.z -= forward.z * CAMERA_SPEED;
    }
    if (IsKeyDown(KEY_LEFT)) {
        camera.position.x += right.x * CAMERA_SPEED;
        camera.position.z += right.z * CAMERA_SPEED;
        camera.target.x += right.x * CAMERA_SPEED;
        camera.target.z += right.z * CAMERA_SPEED;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        camera.position.x -= right.x * CAMERA_SPEED;
        camera.position.z -= right.z * CAMERA_SPEED;
        camera.target.x -= right.x * CAMERA_SPEED;
        camera.target.z -= right.z * CAMERA_SPEED;
    }
    if (IsKeyDown(KEY_W)) {
        camera.position.y += CAMERA_SPEED;
        camera.target.y += CAMERA_SPEED;
    }
    if (IsKeyDown(KEY_S)) {
        camera.position.y -= CAMERA_SPEED;
        camera.target.y -= CAMERA_SPEED;
    }
    if (IsKeyDown(KEY_Z)) {
        Vector3 toTarget = {
            camera.target.x - camera.position.x,
            camera.target.y - camera.position.y,
            camera.target.z - camera.position.z
        };
        float dist = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

        if (dist > 2.0f) {
            toTarget.x /= dist;
            toTarget.y /= dist;
            toTarget.z /= dist;
            camera.position.x += toTarget.x * CAMERA_SPEED * 2;
            camera.position.y += toTarget.y * CAMERA_SPEED * 2;
            camera.position.z += toTarget.z * CAMERA_SPEED * 2;
        }
    }
    if (IsKeyDown(KEY_X)) {
        Vector3 toTarget = {
            camera.target.x - camera.position.x,
            camera.target.y - camera.position.y,
            camera.target.z - camera.position.z
        };
        float dist = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

        if (dist < 50.0f) {
            toTarget.x /= dist;
            toTarget.y /= dist;
            toTarget.z /= dist;
            camera.position.x -= toTarget.x * CAMERA_SPEED * 2;
            camera.position.y -= toTarget.y * CAMERA_SPEED * 2;
            camera.position.z -= toTarget.z * CAMERA_SPEED * 2;
        }
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {

        Vector2 mouseDelta = GetMouseDelta();

        if (fabsf(mouseDelta.x) > 0.1f) {
            float angle = mouseDelta.x * MOUSE_SENSITIVITY;
            Vector3 offset = {
                camera.target.x - camera.position.x,
                camera.target.y - camera.position.y,
                camera.target.z - camera.position.z
            };
            camera.target.x = camera.position.x + offset.x * cosf(angle) - offset.z * sinf(angle);
            camera.target.z = camera.position.z + offset.x * sinf(angle) + offset.z * cosf(angle);
        }

        if (fabsf(mouseDelta.y) > 0.1f) {
            Vector3 offset = {
                camera.target.x - camera.position.x,
                camera.target.y - camera.position.y,
                camera.target.z - camera.position.z
            };

            float newY = offset.y - mouseDelta.y * MOUSE_SENSITIVITY;

            if (newY > -20.0f && newY < 20.0f) {
                camera.target.y = camera.position.y + newY;
            }
        }
    }
}
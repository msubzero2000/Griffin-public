#include "Airplane.h"
#include "Log.h"
#include "Utility.h"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

namespace fly
{

#define MAX_PITCH_ANGLE PI / 6
#define MAX_ROLL_ANGLE PI / 6
#define MAX_ROLL_SPEED 5.0f
#define MAX_FLAPPING_ANGLE 10 * PI / 180
#define MAX_ROLL_VELOCITY 0.2f
#define MAX_PITCH_VELOCITY 0.2f
#define MIN_ALTITUDE 1.0f
#define MAX_ALTITUDE 2.5f

Airplane::Airplane() :
    m_position({0.0f, 0.0f, 1.2f}),
    m_forward({1.f, 0.f, 0.f}),
    m_up({0.f, 0.f, 1.f}),
    m_left({0.f, 1.f, 0.f}),
    m_speed(1.0f),
    m_velocity(glm::vec3(1.f) * m_speed),
    m_translationMatrix(glm::translate(glm::mat4(1.f), m_position)),
    m_rotationMatrix(glm::mat4(1.f)),
    m_rollAngle(0.0f),
    m_pitchAngle(0.0f),
    m_aileron(0),
    m_rollVelocity(0),
    m_elevator(0),
    m_pitchVelocity(0),
    m_throttle(0),
    m_leftWingRoll(0.0f),
    m_leftWingRollVelocity(0.01f),
    m_rightWingRoll(0.0f),
    m_rightWingRollVelocity(0.01f),
    m_targetRollAngle(0.0f),
    m_rightWingRollTarget(0.0f),
    m_leftWingRollTarget(0.0f),
    m_targetBodyHeight(0.0f),
    m_bodyHeight(0.0f),
    m_flying(false),
    m_useTargetRollAngle(true),
    m_model("resources/Body.obj"),
    m_leftWing("resources/Wings-Left.obj"), 
    m_rightWing("resources/Wings-Right.obj"),
    m_tree("resources/Tree.obj")
{
    auto transform = m_translationMatrix;
    m_model.setTransform(transform);
    m_leftWing.setTransform(transform);
    m_rightWing.setTransform(transform);
    
    auto treeTransform = glm::mat4(1.f);
    // Put the tree at the bird start position
    treeTransform = glm::translate(treeTransform, {-0.026f + m_position[0], 0.15f + m_position[1], 0.207f});
    treeTransform = glm::rotate(treeTransform, -PI / 2.0f, {0.0f, 0.0f, 1.0f});
    treeTransform = glm::rotate(treeTransform, PI / 2.0f, {1.0f, 0.0f, 0.0f});
    treeTransform = glm::scale(treeTransform, {0.065f, 0.065f, 0.05f});
    m_tree.setTransform(treeTransform);
    
    m_resetSound = loadSound("/home/gusn/Dev/Dexie-VR/Fly/build/resources/reset.wav", false, m_resetSoundBuffer);
    m_windSound = loadSound("/home/gusn/Dev/Dexie-VR/Fly/build/resources/wind.wav", true, m_windSoundBuffer);
    m_eagleSound = loadSound("/home/gusn/Dev/Dexie-VR/Fly/build/resources/eagle.wav", false, m_eagleSoundBuffer);
}

sf::Sound Airplane::loadSound(std::string soundPath, bool loop, sf::SoundBuffer& soundBuffer) 
{
    // Load sound
    if (!soundBuffer.loadFromFile(soundPath))
    {
        LOG(Error) << "Unable to load sound file" << soundPath << std::endl;
    }    
    sf::Sound sound;
    sound.setBuffer(soundBuffer);
    sound.setLoop(loop);

    return sound;
}

void Airplane::update(float dt)
{
    if (m_flying)
    {
        if (m_throttle)
        {
            m_speed += 0.5 * m_throttle * dt;
            m_speed = std::min(std::max(0.f, m_speed), 3.f);
            m_throttle = 0;
        }

        float target_velocity, rate;
        bool positive_velocity;
        float dAngleX = 0.0f;
        
        if (m_useTargetRollAngle) {
            float rollVel = (m_targetRollAngle - m_rollAngle) / 5.0f;
            if (rollVel > MAX_ROLL_VELOCITY)
            {
                rollVel = MAX_ROLL_VELOCITY;
            } else
            if (rollVel < -MAX_ROLL_VELOCITY)
            {
                rollVel = -MAX_ROLL_VELOCITY;
            }
            dAngleX += rollVel;
        }
        // Limit the roll angle
        if (m_rollAngle >= MAX_ROLL_ANGLE && dAngleX > 0.0f)
        {
            m_rollVelocity = 0.0f;
            target_velocity = 0.0f;
            dAngleX = 0.0f;
        } else if (m_rollAngle <= -MAX_ROLL_ANGLE && dAngleX < 0.0f)
        {
            m_rollVelocity = 0.0f;
            target_velocity = 0.0f;
            dAngleX = 0.0f;
        }

        glm::vec4 cur_forward = glm::normalize(m_rotationMatrix[0]);
        glm::vec4 cur_left    = glm::normalize(m_rotationMatrix[1]);
        glm::vec4 cur_up      = glm::normalize(m_rotationMatrix[2]);

        if (std::abs(dAngleX) > 1e-5)
            m_rollAngle += dAngleX;
            m_rotationMatrix = glm::rotate(m_rotationMatrix, dAngleX, {1.f, 0.f, 0.f});

        if (m_elevator)
        {
            target_velocity = (PI / 4.f * (1.f - ((m_left.z)))) * m_elevator;
            positive_velocity = m_elevator > 0;
            rate = 0.5f;
        }
        else
        {
            target_velocity = -m_pitchVelocity / 5.0f;
            positive_velocity = sign(m_up.z) * sign(m_forward.z) > 0;
            rate = 0.5f;
        }
        
        // Don't fly too high or too low
        if (m_position[2] > MAX_ALTITUDE && m_pitchAngle < 0)
        {
            target_velocity = 0.3f;
            positive_velocity = true;
        } else if (m_position[2] < MIN_ALTITUDE && m_pitchAngle > 0)
        {
            target_velocity = -0.3f;
            positive_velocity = false;
        }
       
        using ClampFunction = const float& (*)(const float&, const float&);

        auto clamping_func = positive_velocity ? ClampFunction(std::min) : ClampFunction(std::max);
        m_pitchVelocity = clamping_func(m_pitchVelocity + target_velocity * rate, target_velocity);
        float dAngleY = m_pitchVelocity * dt;

        // Limit the pitch angle
        if (m_pitchAngle >= MAX_PITCH_ANGLE && dAngleY > 0.0f)
        {
            m_pitchVelocity = 0.0f;
            target_velocity = 0.0f;
            dAngleY = 0.0f;
        } else if (m_pitchAngle <= -MAX_PITCH_ANGLE && dAngleY < 0.0f)
        {
            m_pitchVelocity = 0.0f;
            target_velocity = 0.0f;
            dAngleY = 0.0f;
        }
        char ra[100];
        char pa[100];
        char pos[100];
        sprintf(pa, "%.2f", m_pitchAngle);
        sprintf(ra, "%.2f", m_rollAngle);
        sprintf(pos, "%.2f", m_position[2]);
        //LOG(Info) << "RA: " << ra  << " PA: " << pa << " POSY: " << pos << std::endl;

        if (std::abs(dAngleY) > 1e-5)
            m_rotationMatrix = glm::rotate(m_rotationMatrix, dAngleY, {0.f, 1.f, 0.f});
            m_pitchAngle += dAngleY;

        m_forward = glm::normalize(m_rotationMatrix[0]);
        m_left    = glm::normalize(m_rotationMatrix[1]);
        m_up      = glm::normalize(m_rotationMatrix[2]);

        auto thrust  =  m_forward * 15.0f * m_speed / 1.0f;
        auto drag    = -glm::normalize(m_velocity) * (15.0f / sq(1.0f)) * glm::dot(m_velocity, m_velocity);
        glm::vec3 gravity =  glm::vec3(0, 0, -1) * 2.f;
        glm::vec3 lift    =  {0.f, 0.f, (m_up * (2.f / 1.0f) * sq(glm::dot(m_forward, m_velocity))).z};

        float cosine = std::abs(m_left.z);
        if (cosine >= 0.1)
        {
            float radius = 3.8f / cosine;
            auto centripetal = glm::normalize(glm::vec3{m_up.x, m_up.y, 0.f}) * glm::dot(m_velocity, m_velocity) / radius;
            lift += centripetal;
            auto direction = sign(glm::cross(m_velocity, centripetal).z);
            if (direction !=  0.f)
            {
                // Rotate the model and vectors
                glm::vec3 axis = glm::inverse(m_rotationMatrix) * glm::vec4(0.f, 0.f, direction, 0.f);
                m_rotationMatrix = glm::rotate(m_rotationMatrix, glm::length(m_velocity) / radius * dt, axis);
                m_forward = glm::normalize(m_rotationMatrix[0]);
                m_left    = glm::normalize(m_rotationMatrix[1]);
                m_up      = glm::normalize(m_rotationMatrix[2]);
            }
        }
        
        m_leftWingRoll += m_leftWingRollVelocity;
        // When flying, make the left and right wing syncrhonised
        m_rightWingRoll = m_leftWingRoll;
        
        // Flapping wing animation
        if (m_leftWingRollVelocity > 0.0f && m_leftWingRoll > MAX_FLAPPING_ANGLE) {
            m_leftWingRollVelocity = -0.01f;
        } else
        if (m_leftWingRollVelocity < 0.0f && m_leftWingRoll < -MAX_FLAPPING_ANGLE) {
            m_leftWingRollVelocity = 0.01f;
        }
        
        glm::vec3 acceleration = thrust + drag + gravity + lift;
        m_velocity += acceleration * dt;
        m_position += m_velocity * dt;

        m_translationMatrix = glm::translate(glm::mat4{1.f}, m_position);
    } else {
        m_leftWingRollVelocity = (m_leftWingRollTarget - m_leftWingRoll) / 5.0f;
        m_rightWingRollVelocity = (m_rightWingRollTarget - m_rightWingRoll) / 5.0;
        
        m_leftWingRoll += m_leftWingRollVelocity;
        m_rightWingRoll += m_rightWingRollVelocity;
        
        // Move body up and down
        m_bodyHeight = m_targetBodyHeight * 0.5f + m_bodyHeight * 0.5f;
        
        m_translationMatrix = glm::translate(glm::mat4{1.f}, m_position);
        m_translationMatrix = glm::translate(m_translationMatrix, {0, 0, m_bodyHeight});
    }

    auto transform = m_translationMatrix * m_rotationMatrix;
    m_model.setTransform(transform);
    auto leftWingTransform = transform;
    auto rightWingTransform = transform;

    leftWingTransform = glm::translate(leftWingTransform, {0.013f, 0.005f, 0.014f});
    leftWingTransform = glm::rotate(leftWingTransform, m_leftWingRoll, {1.0f, 0.0f, 0.0f});

    rightWingTransform = glm::translate(rightWingTransform, {0.013f, -0.005f, 0.014f});
    rightWingTransform = glm::rotate(rightWingTransform, -m_rightWingRoll, {1.0f, 0.0f, 0.0f});

    m_leftWing.setTransform(leftWingTransform);
    m_rightWing.setTransform(rightWingTransform);
    
    m_aileron = m_elevator = 0;
}

void Airplane::setTargetRollAngle(int targetRollAngle) 
{
    m_targetRollAngle = ((float)targetRollAngle) * PI / 180.0f;
}

bool Airplane::setGameState(int gameState)
{
    //gameState = 1;
    if (m_flying && gameState == 0)
    {
        // Resetting the game
        m_position[2] = 1.2f; //{0.0f, 0.0f, 1.2f};
        m_forward = {1.f, 0.f, 0.f};
        m_up={0.f, 0.f, 1.f};
        m_left={0.f, 1.f, 0.f};
        m_speed=1.0f;
        m_velocity=glm::vec3(1.f) * m_speed;
        m_translationMatrix=glm::translate(glm::mat4(1.f), m_position);
        m_rotationMatrix=glm::mat4(1.f);
        m_rollAngle=0.0f;
        m_pitchAngle=0.0f;
        m_aileron=0;
        m_rollVelocity=0;
        m_elevator=0;
        m_pitchVelocity=0;
        m_throttle=0;
        m_leftWingRoll=0.0f;
        m_leftWingRollVelocity=0.01f;
        m_rightWingRoll=0.0f;
        m_rightWingRollVelocity=0.01f;
        m_targetRollAngle=0.0f;
        m_rightWingRollTarget=0.0f;
        m_leftWingRollTarget=0.0f;      
        m_targetBodyHeight=0.0f;
        m_bodyHeight=0.0f;
        
        auto transform = m_translationMatrix;
        m_model.setTransform(transform);
        m_leftWing.setTransform(transform);
        m_rightWing.setTransform(transform);

        auto treeTransform = glm::mat4(1.f);
        // Put the tree at the bird start position
        treeTransform = glm::translate(treeTransform, {-0.026f + m_position[0], 0.15f + m_position[1], 0.207f});
        treeTransform = glm::rotate(treeTransform, -PI / 2.0f, {0.0f, 0.0f, 1.0f});
        treeTransform = glm::rotate(treeTransform, PI / 2.0f, {1.0f, 0.0f, 0.0f});
        treeTransform = glm::scale(treeTransform, {0.065f, 0.065f, 0.05f});
        m_tree.setTransform(treeTransform);
        m_windSound.stop();
        m_resetSound.play();
        
        m_flying = false;
        
        return true;
    } else
    if (!m_flying && gameState == 1)
    {
        m_windSound.play();
        m_resetSound.stop();
        m_eagleSound.play();
    }
    m_flying = (gameState == 1);
    
    return false;
}

void Airplane::setLeftWingAngle(int leftWingAngle)
{
    m_leftWingRollTarget = ((float)leftWingAngle) * PI / 180.0f;
}

void Airplane::setRightWingAngle(int rightWingAngle)
{
    m_rightWingRollTarget = ((float)rightWingAngle) * PI / 180.0f;
}

void Airplane::setBodyHeightTarget(int bodyHeightTarget)
{
    m_targetBodyHeight = ((float)bodyHeightTarget) / 3000.0f;
}

}

#ifndef AIRPLANE_H
#define AIRPLANE_H
#include <glm/glm.hpp>
#include <SFML/Audio.hpp>
#include "Model.h"

namespace fly
{


class Airplane
{
public:
    Airplane();
    Model&  getBodyRenderer() { return m_model; }
    Model&  getLeftWingRenderer() { return m_leftWing; }
    Model&  getRightWingRenderer() { return m_rightWing; }
    Model&  getTreeRenderer() { return m_tree; }
    
    void    roll   (char direction) { m_aileron = (direction == 1) ? 1.0f : -1.0f; }
    void    elevate(char direction) { m_elevator = (direction == 1) ? 1.0f : -1.0f; }
    void    throttle(char t)  {  m_throttle = t; }
    void    update(float dt);
    void    recover() {m_position[2] += 1.0f; };
    const   glm::vec3& getPosition() const { return m_position; }
    const   glm::vec3& getForwardDirection() const { return m_forward; }
    const   glm::vec3& getUpDirection() const { return m_up; }
    const   glm::mat4  getModel(){ return m_translationMatrix * m_rotationMatrix; }
    const   AABB&      getLocalBounds() { return m_model.getLocalBounds(); }
    void    crash() { m_model.darken(); }
    void    setTargetRollAngle(int targetRollAngle);
    bool    setGameState(int gameState);
    void    setLeftWingAngle(int leftWingAngle);
    void    setRightWingAngle(int rightWingAngle);
    void    setBodyHeightTarget(int bodyHeight);
 
private:
    sf::Sound loadSound(std::string soundPath, bool loop, sf::SoundBuffer& soundBuffer);

    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;
    glm::vec3 m_left;
    float     m_speed;
    glm::vec3 m_velocity;
    glm::mat4 m_translationMatrix;
    glm::mat4 m_rotationMatrix;
    float      m_aileron;
    float     m_rollVelocity;
    float      m_elevator;
    float     m_pitchVelocity;
    char      m_throttle;
    float     m_rollAngle;
    float     m_pitchAngle;
    Model     m_model;
    Model     m_leftWing;
    Model     m_rightWing;
    Model    m_tree;
    float     m_leftWingRoll;
    float     m_leftWingRollVelocity;
    float     m_rightWingRoll;
    float     m_rightWingRollVelocity;
    float   m_leftWingRollTarget;
    float   m_rightWingRollTarget;
    float     m_targetRollAngle;
    float   m_targetBodyHeight;
    float m_bodyHeight;
    bool    m_useTargetRollAngle;
    bool    m_flying;
    sf::SoundBuffer m_resetSoundBuffer;
    sf::SoundBuffer m_windSoundBuffer;
    sf::SoundBuffer m_eagleSoundBuffer;
    sf::Sound m_resetSound;
    sf::Sound m_windSound;
    sf::Sound m_eagleSound;
};

}

#endif // AIRPLANE_H

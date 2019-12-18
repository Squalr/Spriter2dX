//
// Created by jeremy on 12/12/15.
//
#include <2d/CCSprite.h>
#include <sstream>

#include "AnimationNode.h"
#include "ccobjectfactory.h"

namespace cc = cocos2d;
namespace se = SpriterEngine;

namespace Spriter2dX {
    enum CommandType {PlayOnce, PlayRepeat};

    struct EntityCommand {
        EntityCommand(CommandType type, se::EntityInstance* entity, EntityEvent onComplete)
                : type(type), entity(entity), onComplete(onComplete) {}
        CommandType type;
        std::unique_ptr<se::EntityInstance> entity;
        EntityEvent onComplete;
    };

    void doNothing(se::EntityInstance*) {}

    class AnimationNode::impl {
    public:
        impl(cc::Node* parent, SpriteLoader loader, const std::string& scmlFile)
                : files(new CCFileFactory(parent,loader))
                , model(scmlFile, files, new CCObjectFactory(parent)) {}

        ~impl() {}

        void update(float dt)
        {
            auto removed = std::remove_if(entities.begin(), entities.end(), [dt](const EntityCommand& cmd)
            {
                if (cmd.entity != nullptr)
                {
                    auto pre_ratio = cmd.entity->getTimeRatio();
                    cmd.entity->setTimeElapsed(dt * 1000.0f);
                    
                    if (cmd.type == PlayOnce && (pre_ratio > .99f || pre_ratio > cmd.entity->getTimeRatio()))
                    {
                        cmd.onComplete(cmd.entity.get());
                        return true;
                    }

                    cmd.entity->playAllTriggers();
                    cmd.entity->render();
                }
                return false;
            });
            entities.erase(removed, entities.end());
        }

        se::EntityInstance* createEntity(const std::string &name
                                        ,CommandType type
                                        ,EntityEvent onComplete)
        {
            se::EntityInstance* entity = model.getNewEntityInstance(name);
            entities.emplace_back(type, entity, onComplete);
            return entity;
        }

        void deleteEntity(se::EntityInstance*& remove)
        {
            auto removed = std::remove_if(entities.begin(), entities.end(),
                                          [=](const EntityCommand& cmd){
                                              return cmd.entity.get() == remove;
                                          });
            entities.erase(removed, entities.end());
            remove = nullptr;
        }

        void updateFlipped(bool isFlippedX, bool isFlippedY)
        {
            for (auto it = this->entities.begin(); it != this->entities.end(); *it++)
            {
                if (it->entity != nullptr)
                {
                    it->entity->setScale(SpriterEngine::point(isFlippedX ? -1.0f : 1.0f, isFlippedY ? -1.0f : 1.0f));
                }
            }
        }

    private:
        CCFileFactory* files;
        std::vector<EntityCommand> entities;
        se::SpriterModel model;
    };

    se::EntityInstance *createEntity(const std::string &name, CommandType type);


    AnimationNode::AnimationNode(const std::string& scmlFile, SpriteLoader loader)
            : self(new impl(this, loader, scmlFile)), isFlippedX(false), isFlippedY(false) {}

    AnimationNode::~AnimationNode() {}

    void AnimationNode::update(float dt)
    {
        self->update(dt);
    }

    void AnimationNode::setFlippedX(bool flippedX)
    {
        this->isFlippedX = flippedX;
        this->self->updateFlipped(this->isFlippedX, this->isFlippedY);
    }

    void AnimationNode::setFlippedY(bool flippedY)
    {
        this->isFlippedY = flippedY;
        this->self->updateFlipped(this->isFlippedX, this->isFlippedY);
    }

    se::EntityInstance* AnimationNode::playOnce(const std::string &name)
    {
        SpriterEngine::EntityInstance* instance = self->createEntity(name, PlayOnce, doNothing);
        this->self->updateFlipped(this->isFlippedX, this->isFlippedY);
        return instance;
    }

    se::EntityInstance* AnimationNode::playOnce(const std::string &name
                                               ,EntityEvent onComplete)
    {
        SpriterEngine::EntityInstance* instance = self->createEntity(name, PlayOnce, onComplete);
        this->self->updateFlipped(this->isFlippedX, this->isFlippedY);

        return instance;
    }

    SpriterEngine::EntityInstance *AnimationNode::play(const std::string &name) {
        SpriterEngine::EntityInstance* instance = self->createEntity(name, PlayRepeat, doNothing);
        this->self->updateFlipped(this->isFlippedX, this->isFlippedY);

        return instance;
    }

    AnimationNode* AnimationNode::create(const std::string& scmlFile, SpriteLoader loader)
    {
        AnimationNode* ret = new (std::nothrow) AnimationNode(scmlFile, loader);
        if (ret && ret->init())
        {
            ret->autorelease();
        }
        else
        {
            CC_SAFE_DELETE(ret);
        }
        return ret;
    }

    SpriteLoader AnimationNode::fileLoader()
    {
        return [](const std::string& name) { return cc::Sprite::create(name);};
    }

    std::vector<std::string> split(const std::string& s, char delim)
    {
        std::stringstream ss(s);
        std::string item;
        std::vector<std::string> elems;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }

    SpriteLoader AnimationNode::cacheLoader()
    {
        return [](const std::string& name) {
            auto fullpath = split(name, '/');
            return cc::Sprite::createWithSpriteFrameName(fullpath.back());
        };
    }

    void AnimationNode::onEnter()
    {
        cc::Node::onEnter();
        this->scheduleUpdate();
    }

    void AnimationNode::onExit()
    {
        cc::Node::onExit();
        this->unscheduleUpdate();
    }

    void AnimationNode::deleteEntity(se::EntityInstance*& remove)
    {
        self->deleteEntity(remove);
    }

}

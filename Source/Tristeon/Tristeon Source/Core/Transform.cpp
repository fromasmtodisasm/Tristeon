﻿#include "Transform.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace Tristeon
{
	namespace Core
	{
		//TODO: Global rotation and scale are untested. Parent transformations are untested

		REGISTER_TYPE_CPP(Transform)

		Transform::~Transform()
		{
			//Get rid of all relationships
			for (int i = 0; i < children.size(); i++)
				children[i]->parent = nullptr;
			children.clear();

			if (parent != nullptr)
				parent->children.remove(this);
			parent = nullptr;
		}

		void Transform::setParent(Transform* parent, bool keepWorldTransform)
		{
			//Can't parent to ourselves
			if (parent == this)
				return;

			//Deregister ourselves from our old parent
			if (this->parent != nullptr)
				this->parent->children.remove(this);
			//Add ourselves to the new parent
			if (parent != nullptr)
				parent->children.push_back(this);

			//Transformation
			if (keepWorldTransform)
			{
				//Store old transformation
				Math::Vector3 const oldGlobalPos = position;
				Math::Vector3 const oldGlobalScale = scale;
				Math::Quaternion const oldGlobalRot = rotation;

				//Apply
				this->parent = parent;

				//Reset transform
				position = oldGlobalPos;
				scale = oldGlobalScale;
				rotation = oldGlobalRot;
			}
			//Apply
			else
			{
				this->parent = parent;
			}
		}

		nlohmann::json Transform::serialize()
		{
			nlohmann::json output;
			output["typeID"] = typeid(Transform).name();
			output["parentID"] = parent == nullptr ? -1 : parent->getInstanceID();
			output["localPosition"] = _localPosition.serialize();
			output["localScale"] = _localScale.serialize();
			output["localRotation"] = _localRotation.eulerAngles().serialize();
			return output;
		}

		void Transform::deserialize(nlohmann::json json)
		{
			_localPosition.deserialize(json["localPosition"]);
			_localScale.deserialize(json["localScale"]);
			Math::Vector3 eulerAngles;
			eulerAngles.deserialize(json["localRotation"]);
			_localRotation = Math::Quaternion::euler(eulerAngles);
		}

		Math::Vector3 Transform::transformPoint(Math::Vector3 point) const
		{
			glm::vec4 const res = glm::vec4(point.x, point.y, point.z, 1.0) * getTransformationMatrix();
			return{ res.x, res.y, res.z };
		}

		Math::Vector3 Transform::inverseTransformPoint(Math::Vector3 point) const
		{
			glm::vec4 const res = glm::vec4(point.x, point.y, point.z, 1.0) *  inverse(getTransformationMatrix());
			return{ res.x, res.y, res.z };
		}

		Math::Vector3 Transform::getGlobalPosition() const
		{
			if (parent == nullptr)
				return _localPosition;
			else
				return Vec_Convert3(getTransformationMatrix()[3]);
		}

		void Transform::setGlobalPosition(Math::Vector3 pos)
		{
			if (parent == nullptr)
				_localPosition = pos;
			else
				_localPosition = parent->inverseTransformPoint(pos);
		}

		Math::Vector3 Transform::getGlobalScale() const
		{
			if (parent == nullptr)
				return _localScale;
			
			//Get scale off our transformation matrix
			glm::mat4 const trans = getTransformationMatrix();
				
			glm::vec3 scale;
			//Unused variables but required in the function
			glm::quat rotation;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;
			decompose(trans, scale, rotation, translation, skew, perspective);

			return Vec_Convert3(scale);
		}

		void Transform::setGlobalScale(Math::Vector3 scale)
		{
			if (parent == nullptr)
				_localScale = scale;
			else
			{
				glm::mat4 const p = parent->getTransformationMatrix();

				glm::vec3 s;
				glm::quat r;
				glm::vec3 t;
				glm::vec3 sk;
				glm::vec4 per;
				decompose(p, s, r, t, sk, per);

				//New localscale, defined as x
				//x * parent = goal //When we multiply our new localscale with parent it should equal to our goal
				//x = goal / parent //Which means that we can define x as this
				//so 
				//_localScale = scale / parent
				_localScale = scale / Math::Vector3(s.x, s.y, s.z);
			}
		}

		Math::Quaternion Transform::getGlobalRotation() const
		{
			if (parent == nullptr)
				return _localRotation;
			
			//Get global rotation off our matrix
			glm::mat4 const trans = getTransformationMatrix();
			glm::quat rotation;

			//Unused variables, required for the function
			glm::vec3 scale;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;
			decompose(trans, scale, rotation, translation, skew, perspective);

			return Math::Quaternion(rotation);
		}

		void Transform::setGlobalRotation(Math::Quaternion rot)
		{
			if (parent == nullptr)
				_localRotation = rot;
			else
			{
				//Throwaway values
				glm::vec3 scale;
				glm::vec3 translation;
				glm::vec3 skew;
				glm::vec4 perspective;

				//Get parent info
				glm::mat4 const p = parent->getTransformationMatrix();
				glm::quat rotation;
				decompose(p, scale, rotation, translation, skew, perspective);

				glm::mat4 const pr = glm::mat4(rotation);			//Parent rotation
				glm::mat4 const gl = glm::mat4(rot.getGLMQuat());	//Goal rotation

				//Local needs to be x so that
				//pr * x = gl
				//So x = gl / pr
				glm::mat4 const x = gl / pr;
				decompose(x, scale, rotation, translation, skew, perspective);
				
				//Local rotation
				glm::quat const local = rotation;
				_localRotation = Math::Quaternion(local);
			}
		}

		glm::mat4 Transform::getTransformationMatrix() const
		{
			//TODO: Cache local transformation [Optimization]

			//Get parent transformation (recursive)
			glm::mat4 p = glm::mat4(1.0f);
			if (parent != nullptr)
				p *= parent->getTransformationMatrix();

			//Get transformation
			glm::mat4 const t = glm::translate(glm::mat4(1.0f), Vec_Convert3(position));
			glm::mat4 const r = glm::mat4(rotation.getGLMQuat());
			glm::mat4 const s = glm::scale(glm::mat4(1.0f), Vec_Convert3(scale));

			//Apply and return
			return t * r * s * p;
		}

		void Transform::rotate(Math::Vector3 axis, float rot)
		{
			rotation = rotation.rotate(axis, rot);
		}

		void Transform::translate(Math::Vector3 t)
		{
			position += inverseTransformPoint(t);
		}

		Transform* Transform::getParent() const
		{
			return parent;
		}
	}
}
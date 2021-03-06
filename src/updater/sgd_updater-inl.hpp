#ifndef TEXTNET_SGD_UPDATER_INL_HPP_
#define TEXTNET_SGD_UPDATER_INL_HPP_

#include <iostream>
#include <mshadow/tensor.h>
#include "./updater.h"

namespace textnet {
namespace updater {

template<typename xpu, int dim>
class SGDUpdater : public Updater<xpu, dim>{
 public:
  SGDUpdater(std::map<std::string, SettingV> &setting, 
                      mshadow::Random<xpu>* prnd) {
    this->prnd_ = prnd;
    SetupUpdater(setting);
  }
  virtual ~SGDUpdater(void) {}
  
  virtual void Require(std::map<std::string, SettingV> &setting) {
    // default value, just set the value you want
    this->defaults["decay"] = SettingV(0.0f);
    this->defaults["momentum"] = SettingV(0.0f);
    this->defaults["l2"] = SettingV(0.0f);
    this->defaults["batch_size"] = SettingV(1);

    // require value, set to SettingV(),
    // it will force custom to set in config
    this->defaults["lr"] = SettingV();
    
    Updater<xpu, dim>::Require(setting);
  }
  
  virtual void SetupUpdater(std::map<std::string, SettingV> &setting) {
    Updater<xpu, dim>::SetupUpdater(setting);
    
    this->updater_type = setting["updater_type"].iVal();
    batch_size = setting["batch_size"].iVal(); 
    base_lr = setting["lr"].fVal();
    decay = setting["decay"].fVal();
    momentum = setting["momentum"].fVal();
    l2 = setting["l2"].fVal();
    iteration = 0;
	  lr = base_lr;
  }
  
  virtual void Update(mshadow::Tensor<xpu, dim> data, 
                      mshadow::Tensor<xpu, dim> diff) {
    if (momentum != 0.0 && iteration == 0) {
      history.Resize(data.shape_, 0);
    }
    if (batch_size > 1) {
        diff /= float(batch_size);
    }
                        
    AdaptLearningRate();
    iteration++;
    
    if (momentum == 0.0) {
      data -= lr * (diff + l2 * data);
    } else {
      history = lr * (diff + l2 * data) + momentum * history;
      data -= history;
    }
  }
  
  virtual void UpdateSparse(mshadow::Tensor<xpu, dim> data, 
                            mshadow::Tensor<xpu, dim> diff, 
                            mshadow::Tensor<xpu, 1> idx) {
    if (momentum != 0.0 && iteration == 0) {
      history.Resize(data.shape_, 0);
    }
    if (batch_size > 1) {
        diff /= float(batch_size);
    }
    
    AdaptLearningRate();
    iteration++;

    if (momentum == 0.0) {
      int w_idx = -1;
      for (int i = 0; i < idx.size(0); ++i) {
        w_idx = idx[i];
        data[w_idx] -= lr * (diff[i] + l2 * data[w_idx]);
      }
    } else {
      int w_idx = -1;
      for (int i = 0; i < idx.size(0); ++i) {
        w_idx = idx[i];
        history[w_idx] = lr * (diff[i] + l2 * data[w_idx]) + momentum * history[w_idx];
        data[w_idx] -= history[w_idx];
      }
    }
  }
  
  virtual void AdaptLearningRate() {
	if (lr < 0.1 * base_lr) return;
    lr = base_lr * (1.0 - decay * iteration);
  }
  
 protected: 
  float momentum;
  mshadow::TensorContainer<xpu, dim> history;
  int iteration, batch_size;
  float lr;
  float base_lr;
  float decay;
  float l2;
};
}  // namespace updater
}  // namespace textnet
#endif  // TEXTNET_CONSTANT_INIT_INL_HPP_


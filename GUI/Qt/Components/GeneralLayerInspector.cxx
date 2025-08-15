#include "GeneralLayerInspector.h"
#include "ui_GeneralLayerInspector.h"

#include "LayerGeneralPropertiesModel.h"
#include "LayerInspectorRowDelegate.h"

#include "QtCheckBoxCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "TagListWidgetCoupling.h"
#include "MeshWrapperBase.h"
#include "StandaloneMeshWrapper.h"

#include "QtWidgetActivator.h"
#include "QtRadioButtonCoupling.h"

#include "TagListWidget.h"

Q_DECLARE_METATYPE(LayerGeneralPropertiesModel::DisplayMode)

using MeshDataType = LayerGeneralPropertiesModel::MeshDataType;


// Needed for coupling active mesh array model with ComboBox
template <>
class DefaultComboBoxRowTraits<int, MeshDataArrayDescriptionTraits::Value>
  : public TextAndIconComboBoxRowTraits<int, MeshDataArrayDescriptionTraits::Value, MeshDataArrayDescriptionTraits>
{};


// Needed for coupling active mesh array model with ComboBox
template <>
class DefaultComboBoxRowTraits<vtkIdType, MeshVectorComponentDescriptionTraits::Value>
  : public TextAndIconComboBoxRowTraits<vtkIdType, MeshVectorComponentDescriptionTraits::Value, MeshVectorComponentDescriptionTraits>
{};


std::map<int, const char*>
GeneralLayerInspector::MeshDataTypeToIcon =
{
	{ MeshDataType::POINT_DATA, ":/root/mesh_point_data.png" },
	{ MeshDataType::CELL_DATA, ":/root/mesh_cell_data.png" }
};

GeneralLayerInspector::GeneralLayerInspector(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::GeneralLayerInspector)
{
  ui->setupUi(this);
}

GeneralLayerInspector::~GeneralLayerInspector()
{
  delete ui;
}

void GeneralLayerInspector::SetModel(LayerGeneralPropertiesModel *model)
{
  m_Model = model;

  // Couple the widgets
  makeCoupling(ui->inMode, m_Model->GetDisplayModeModel());
  makeCoupling(ui->inComponent, m_Model->GetSelectedComponentModel());
  makeCoupling(ui->inComponentSlider, m_Model->GetSelectedComponentModel());
  makeCoupling((QAbstractButton *)ui->btnAnimate, m_Model->GetAnimateModel());
  makeCoupling(ui->sliderTP, m_Model->GetParentModel()->GetCursorTimePointModel());
  makeCoupling(ui->spinBoxTP, m_Model->GetParentModel()->GetCursorTimePointModel());
  makeCoupling(ui->inTPNickname, m_Model->GetCrntTimePointNicknameModel());
  makeCoupling(ui->TPTagsWidget, m_Model->GetCrntTimePointTagListModel());
  makeCoupling((QAbstractButton *)ui->btn4DReplay, m_Model->GetParentModel()->GetGlobalState()->Get4DReplayModel());
  makeCoupling(ui->in4DReplayInterval, m_Model->GetParentModel()->GetGlobalState()->Get4DReplayIntervalModel());

  auto validator = new QIntValidator(this);
  validator->setRange(1, 60000); // who would need >1min interval?
  ui->in4DReplayInterval->setValidator(validator);

  // Couple the pin/unpin buttons
  std::map<bool, QAbstractButton *> button_map;
  button_map[true] = ui->btnPin;
  button_map[false] = ui->btnUnpin;
  makeCheckableWidgetGroupCoupling(ui->grpOverlayChecks, button_map, m_Model->GetIsStickyModel());

  makeCoupling(ui->inOpacity, m_Model->GetLayerOpacityModel());
  makeCoupling(ui->inOpacityValue, m_Model->GetLayerOpacityModel());
  makeCoupling((QAbstractButton *)ui->btnVisible, m_Model->GetLayerVisibilityModel());

  makeCoupling(ui->outFilename, m_Model->GetFilenameModel());
  makeCoupling(ui->inNickname, m_Model->GetNicknameModel());

  makeCoupling(ui->tagsWidget, m_Model->GetTagsModel());

  activateOnFlag(ui->grpMulticomponent, m_Model,
                 LayerGeneralPropertiesModel::UIF_MULTICOMPONENT,
                 QtWidgetActivator::HideInactive);

  activateOnFlag(ui->grp4DProperties, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_4D_IMAGE,
                 QtWidgetActivator::HideInactive);

  activateOnFlag(ui->grpComponent, m_Model,
                 LayerGeneralPropertiesModel::UIF_CAN_SWITCH_COMPONENTS);

  activateOnFlag(ui->grpSecondary, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_STICKINESS_EDITABLE,
                 QtWidgetActivator::HideInactive);

  activateOnFlag(ui->grpOpacity, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_OPACITY_EDITABLE);
  activateOnFlag(ui->lblOpacity, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_OPACITY_EDITABLE);

  activateOnFlag(ui->btnUp, m_Model,
                 LayerGeneralPropertiesModel::UIF_MOVABLE_UP);
  activateOnFlag(ui->btnDown, m_Model,
                 LayerGeneralPropertiesModel::UIF_MOVABLE_DOWN);

	// Group Mesh should only display when a mesh layer with data is selected
  activateOnFlag(ui->grpMesh, m_Model,
								 LayerGeneralPropertiesModel::UIF_MESH_HAS_DATA,
                 QtWidgetActivator::HideInactive);

	// Mesh vector data connection
  activateOnFlag(ui->lblMeshVectorMode, m_Model,
								 LayerGeneralPropertiesModel::UIF_IS_MESHDATA_MULTICOMPONENT,
								 QtWidgetActivator::HideInactive);
  activateOnFlag(ui->boxMeshVectorMode, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_MESHDATA_MULTICOMPONENT,
                 QtWidgetActivator::HideInactive);

  // Solid color button: always on because it controls the slice views too
  /*
  activateOnFlag(ui->lblMeshSolidColor, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_MESHDATA_SOLID_COLOR,
                 QtWidgetActivator::HideInactive);
  activateOnFlag(ui->boxMeshSolidColor, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_MESHDATA_SOLID_COLOR,
                 QtWidgetActivator::HideInactive);
  */

  makeCoupling(ui->boxMeshDataName, m_Model->GetActiveMeshLayerDataPropertyIdModel());
  makeCoupling(ui->boxMeshVectorMode, m_Model->GetMeshVectorModeModel());
  makeCoupling(ui->btnMeshSolidColor, m_Model->GetMeshSolidColorModel());
  makeCoupling(ui->inMeshSliceOpacity, m_Model->GetMeshSliceViewOpacityModel());
}

void GeneralLayerInspector::on_btnUp_clicked()
{
  m_Model->MoveLayerUp();
}

void GeneralLayerInspector::on_btnDown_clicked()
{
  m_Model->MoveLayerDown();
}

void GeneralLayerInspector::on_spinBoxTP_valueChanged(int value)
{
  QString txt("Properties for Time Point ");
  txt.append(std::to_string(value).c_str());
  txt.append(":");
  ui->grpTPProperties->setTitle(txt);
}

void GeneralLayerInspector::onModelUpdate(const EventBucket &)
{
}

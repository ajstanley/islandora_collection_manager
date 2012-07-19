<?php

function islandora_collection_change_contentmodel(&$form_state, $pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
  $represented_content_models = get_represented_content_models($pid);
  $new_content_models = get_content_models_as_option_array();
  $form = array();

  $form['link'] = array(
    '#type' => 'item',
    '#value' => l(t("Return to collection view"), "fedora/repository/$pid"),
  );

  $form['titlebox'] = array(
    '#type' => 'item',
    '#value' => t("Change Content Models within $pid"),
  );

  $form['current_content_model'] = array(
    '#title' => "Choose content model to be changed",
    '#type' => 'select',
    '#options' => $represented_content_models,
    '#description' => t("All objects in this collection with the selected content model will be changed."),
  );
  $form['new_content_model'] = array(
    '#title' => "Choose new content model",
    '#type' => 'select',
    '#options' => $new_content_models,
    '#description' => t("The new content model to be assigned to selected objects."),
  );
  $form['collection_pid'] = array(
    '#type' => 'hidden',
    '#value' => $pid,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Change Content Model Associations'),
    '#id' => 'change_model',
  );
  return $form;
}

function islandora_collection_change_contentmodel_submit($form, &$form_state) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
  $current_content_model = $form_state['values']['current_content_model'];
  $new_content_model = $form_state['values']['new_content_model'];
  $collection_pid = $form_state['values']['collection_pid'];
  $policy = CollectionPolicy::loadFromCollection($collection_pid, TRUE);
  $add_to_policy = TRUE;
  $policy_cms = $policy->getContentModels();
  foreach ($policy_cms as $policy_cm) {
    if ($policy_cm->pid == $current_content_model) {
      $namespace = $policy_cm->pid_namespace;
    }
    if ($policy_cm->pid == $new_content_model) {
      $add_to_policy = FALSE;
    }
  }
  if ($add_to_policy) {
    $policy->addModel(ContentModel::loadFromModel($new_content_model), $namespace);
    $policy->saveToFedora();
  }
  $query = "select \$object  from <#ri>
    where (\$object <info:fedora/fedora-system:def/model#hasModel> <info:fedora/$current_content_model>
      and (\$object <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/$collection_pid>
           or \$object <info:fedora/fedora-system:def/relations-external#isMemberOf> <info:fedora/$collection_pid>)
      and \$object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>)";
  $query = htmlentities(urlencode($query));
  $content = '';

  $url = variable_get('fedora_repository_url', 'http://localhost:8080/fedora/risearch');
  $url .= "?type=tuples&flush=TRUE&format=csv&limit=$limit&offset=$offset&lang=itql&stream=on&query=" . $query;
  $content .= do_curl($url);
  $results = explode("\n", $content);
  $object_pids = preg_replace('/^info:fedora\/|"object"/', '', $results);
  $count = 0;
  foreach ($object_pids as $object_pid) {
    if (!$object_pid) {
      continue;
    }
    $item = new fedora_item($object_pid);
    $item->purge_relationship('hasModel', $current_content_model);
    $item->add_relationship('hasModel', $new_content_model, FEDORA_MODEL_URI);
    $count++;
  }
  drupal_set_message("$current_content_model changed to $new_content_model on $count objects");
}

/**
 * returns content models associated with all objects in a collection
 * @param string $pid
 * @return arrau
 */
function get_represented_content_models($pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $query = "select \$model from <#ri>
where (\$object <info:fedora/fedora-system:def/relations-external#isMemberOf> <info:fedora/$pid>
       or \$object <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/$pid>)
       and \$object <info:fedora/fedora-system:def/model#hasModel> \$model";
  $query = htmlentities(urlencode($query));
  $content = '';
  $url = variable_get('fedora_repository_url', 'http://localhost:8080/fedora/risearch');
  $url .= "?type=tuples&flush=TRUE&format=csv&limit=$limit&offset=$offset&lang=itql&stream=on&query=" . $query;
  $content .= do_curl($url);
  $results = explode("\n", $content);
  $model_pids = preg_replace('/^info:fedora\/|"model"/', '', $results);
  $represented_models = array();
  foreach ($model_pids as $model_pid) {
    if ($model_pid && $model_pid != 'fedora-system:FedoraObject-3.0') {
      $item = new fedora_item($model_pid);
      $label = $item->objectProfile->objLabel;
      $represented_models[$model_pid] = "$model_pid ~ $label";
    }
  }
  return $represented_models;
}
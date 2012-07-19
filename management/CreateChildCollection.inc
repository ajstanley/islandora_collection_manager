<?php

function islandora_collection_create_child(&$form_state, $pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $collection_namespace = $current_vals[$cm->pid]['namespace'] = substr($pid, 0, strpos($pid, ":"));
  $form = array();

  $form['link'] = array(
    '#type' => 'item',
    '#value' => l(t("Return to collection view"), "fedora/repository/$pid"),
  );

  $form['titlebox'] = array(
    '#type' => 'item',
    '#value' => t("Create New Child Collection within $pid"),
  );

  $form['collection_name'] = array(
    '#title' => "Collection Name",
    '#type' => 'textfield',
    '#description' => t("Human readable name for new collection"),
  );

  $form['new_collection_pid'] = array(
    '#title' => "Collection PID",
    '#type' => 'textfield',
    '#size' => 15,
    '#default_value' => Fedora_Item::get_next_PID_in_namespace($collection_namespace),
    '#description' => t("Unique PID for this collection.  <br />Pids take the general form of namespace:collection eg islandora:pamphlets"),
  );


  $form['collection_pid'] = array(
    '#type' => 'hidden',
    '#value' => $pid,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create Collection'),
  );
  return $form;
}

/**
 * collection creation form validate
 * @param array $form
 * @param array $form_state
 */
function islandora_collection_create_child_validate($form, &$form_state) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
  $pid = $form_state['values']['new_collection_pid'];
  $item = new fedora_item($pid);
  if ($item->exists()) {
    form_set_error('new_collection_pid', t("$pid already exists within your repository. the PID must be unique. Click on 'Manage This Collection' tab and enter new value."));
    return;
  }
  if (!valid_pid($pid)) {
    form_set_error('new_collection_pid', t("$pid is not a valid identifier. Click on 'Manage This Collection' tab and enter new value."));
    return;
  }
}

function islandora_collection_create_child_submit($form, &$form_state) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'islandor_collection_manager', 'CollectionPolicyManagement');
  $module_path = drupal_get_path('module', 'fedora_repository');
  $thumbnail = drupal_get_path('module', 'Fedora_Repository') . '/images/Crystal_Clear_filesystem_folder_grey.png';
  $new_collection_pid = $form_state['values']['new_collection_pid'];
  $new_collection_label = $form_state['values']['collection_name'];
  $collection_pid = $form_state['values']['collection_pid'];
  $collection_policy = get_blank_collection_policy();
  $item = fedora_item::ingest_new_item($new_collection_pid, 'A', $new_collection_label, $user->name);
  $item->add_relationship('isMemberOfCollection', $collection_pid, RELS_EXT_URI);
  $item->add_relationship('hasModel', 'islandora:collectionCModel', FEDORA_MODEL_URI);
  $item->add_datastream_from_string($collection_policy, 'COLLECTION_POLICY', 'COLLECTION_POLICY', 'text/xml', 'X');
  $item->add_datastream_from_file($thumbnail, 'TN');
  drupal_set_message(t("Define the Collection Policy for <strong>$new_collection_label</strong>."));

  drupal_goto("islandora/collection/management/collection policy/$new_collection_pid");
}
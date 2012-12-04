<?php
/**
 * @file
 *   Holds the code related to deleting objects/collections for this module.
 */

/**
 * The form for deleting collections and their members.
 *
 * @param array $form_state
 *   The state of the form
 * @param string $pid
 *   The collection PID
 *
 * @return array
 *   The completed form array.
 */
function islandora_collection_deletion_form(&$form_state, $pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
  module_load_include('inc', 'islandora_collection_manager', 'CollectionManagerTable');
  module_load_include('inc', 'islandora_collection_manager', 'CollectionManagement');
  $potential_collections = get_collections_as_option_array();
  $table = islandora_collection_table($pid);
  $deletion_message = ($table) ? "Delete Members of this Collection" : "Delete Collection";
  $submit_text_message = ($table) ? "Delete Selected Objects" : "Delete Collection";
    $path = drupal_get_path('module', 'islandora_collection_manager');
    drupal_add_js($path . '/js/collection.js?stopcache');
    drupal_add_css($path . '/css/collection.css');

  $form = array();

  $form['link'] = array(
    '#type' => 'item',
    '#value' => l(t("Return to collection view"), "fedora/repository/$pid"),
  );

  $form['collection_delete'] = array(
    '#title' => $deletion_message,
    '#type' => 'fieldset',
  );

  if ($table) {
    $form['collection_delete']['table'] = array(
      'table' => $table,
    );
  }
  else {
    $form['collection_delete']['delete_root'] = array(
      '#type' => 'checkbox',
      '#title' => "Remove this empty collection?",
      '#id' => 'delete_collection',
    );
  }
  $form['current'] = array(
    '#type' => 'hidden',
    '#value' => $pid,
  );

  $form['collection_delete']['message'] = array(
    '#type' => 'item',
    '#value' => t("This action is permanant and cannot be undone."),
  );
  $form['collection_delete']['submit'] = array(
    '#type' => 'submit',
    '#description' => t("This action is permanant and cannot be undone."),
    '#value' => t($submit_text_message),
  );

  if (user_access('delete entire collections') && user_access('purge objects and datastreams')) {
    return $form;
  }
}

/**
 * The submit hook for the deletion form. Actualy deletes objects
 *
 * @param array $form
 *   The form to handle.
 * @param array $form_state
 *   The state of the form to handle
 */
function islandora_collection_deletion_form_submit($form, &$form_state) {
  global $user;
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');

  $collection_pid = $form_state['values']['current'];
  $parents = get_parent_collections_from_pid($collection_pid);

  if ($form_state['values']['delete_root'] == 1) {
    delete_root_collection($collection_pid);
    drupal_goto("fedora/repository/" . $parents[0]);
  }

  $child_collections = get_child_collections($collection_pid);

  $populated_child_collections = array();
  $pids = @array_filter($form_state['values']['table']['selections']);

  if (!empty($child_collections)) {
    foreach ($child_collections as $child) {
      $child_pids = get_related_items_as_array($child, 'isMemberOfCollection');
      if (!empty($child_pids)) {
        $populated_child_collections[] = $child;
      }
    }
  }
  if (!empty($populated_child_collections)) {
    $conflict = FALSE;
    foreach ($populated_child_collections as $collection) {
      if (in_array($collection, $pids)) {
        $conflict = TRUE;
        drupal_set_message("Populated child collections were not deleted.");
      }
    }
  }
  $pids_to_delete = array_diff($pids, $populated_child_collections);
  $batch = array(
    'title' => "Deleting Selected Objects from collection",
    'progress_message' => t('Deleted @current objects out of @total.'),
    'operations' => array(),
    'file' => drupal_get_path('module', 'islandora_collection_manager') . '/management/DeleteCollection.inc',
  );

  foreach ($pids_to_delete as $pid_to_delete) {
    $batch['operations'][] = array('collection_manager_delete', array($pid_to_delete));
  }
  batch_set($batch);
}

/**
 * This function will delete a collection.
 *
 * @param string $pid
 *   The PID of the item to purge.
 */
function delete_root_collection($pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $item = new Fedora_Item($pid);
  $item->purge();
  drupal_set_message("$pid deleted.");
}

/**
 *  This function will delete an object.
 *
 * @param string $pid
 *   The PID of the item to purge.
 */
function collection_manager_delete($pid) {
  global $user;
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $item = new Fedora_Item($pid);
  $item->purge("Purged by " . $user->name);
  $item->forget();
}

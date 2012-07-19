<?php

function islandora_collection_add_query(&$form_state, $pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $item = new Fedora_Item($pid);
  $label = $item->objectProfile->objLabel;
  $hasQuery = array_key_exists('QUERY', $item->datastreams);
  $query = '';
  $identifier = "$label <em>($pid)</em>";
  if (!$hasQuery) {
    $prompt = t("Add custom query to collection !pid", array('!pid' => $identifier));
  }
  else {
    $prompt = t("Modify or remove custom query from !pid", array('!pid' => $identifier));
    $query = $item->get_datastream_dissemination('QUERY');
  }
  $form = array();

  $form['link'] = array(
    '#type' => 'item',
    '#value' => l(t("Return to collection view"), "fedora/repository/$pid"),
  );

  $form['pid'] = array(
    '#type' => 'hidden',
    '#value' => $pid,
  );
  $form['titlebox'] = array(
    '#type' => 'item',
    '#value' => "<strong>$prompt</strong>",
  );
  $form['query'] = array(
    '#type' => 'textarea',
    '#default_value' => $query,
    '#rows' => 10,
  );
  if ($hasQuery) {
    $form['remove'] = array(
      '#title' => t("Remove Custom Query?"),
      '#description' => t("Checking this box will remove the current custom query.  Collection will revert to default Islandora query"),
      '#type' => 'checkbox'
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
  );
  return $form;
}

function islandora_collection_add_query_submit($form, &$form_state) {
  global $base_url;
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $pid = $form_state['values']['pid'];
  $item = new Fedora_Item($pid);
  if ($form_state['values']['remove']) {
    $item->purge_datastream('QUERY');
    drupal_set_message(t('QUERY datastream removed from the collection'));
  }
  else {
    $new_query = $form_state['values']['query'];
    $flat_pid = str_replace(':', '_', $pid);
    $file_path = file_directory_path() . "/$flat_pid";
    $file = "$file_path/query.txt";
    if (!is_dir($file_path)) {
      mkdir($file_path, 0777, TRUE);
    }
    file_put_contents($file, $new_query);
    $success = $item->add_or_modify_by_reference("$base_url/$file", 'QUERY', "QUERY", 'text/plain', FALSE, 'Modified by Islandora API');
    drupal_set_message(t('QUERY datastream updated'));
    file_delete("$file");
    rmdir($file_path);
  }
  drupal_goto("fedora/repository/$pid");
}
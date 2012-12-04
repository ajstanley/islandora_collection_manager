<?php

function collection_policy_management($form_state, $pid) {
  $table = islandora_collectionPolicy_table($pid);
  $alink_options = array(
    'attributes' => array('target' => '_blank'),
    'html' => TRUE,
  );
  $form = array();

  $form['link'] = array(
    '#type' => 'item',
    '#value' => l(t("Return to collection view"), "fedora/repository/$pid"),
  );


  // place holder until we can establish a collection policy help file

  $form['help_link'] = array(
    '#value' => l(t('About Collection Policies'), 'https://wiki.duraspace.org/display/ISLANDORA112/How+to+Manage+Collection+Policies', $alink_options),
  );
  $form['pid'] = array(
    '#type' => 'hidden',
    '#value' => $pid,
  );
  $form['reorder_pages']['table'] = array(
    'table' => $table,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update Collection Policy'),
  );
  return $form;
}

function collection_policy_management_submit($form, &$form_state) {
  module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $pid = $form_state['values']['pid'];
  $item = new Fedora_Item($pid);
  $chosen = array_filter($form_state['values']['table']['selections']);
  $collection_policy = get_blank_collection_policy();
  $collection_policy_xml = simplexml_load_string($collection_policy);
  foreach ($form_state['values']['table']['rows'] as $candidate) {
    if (in_array($candidate['identifier'], $chosen)) {
      $node = $collection_policy_xml->content_models->addChild('content_model');
      $node->addAttribute('dsid', 'ISLANDORACM');
      $node->addAttribute('name', $candidate['prompt']);
      $node->addAttribute('namespace', $candidate['namespace'] . ':1');
      $node->addAttribute('pid', $candidate['identifier']);
    }
  }

  $item->modify_datastream_by_value($collection_policy_xml->saveXML(), 'COLLECTION_POLICY', 'COLLECTION_POLICY', 'text/xml');
  drupal_goto("fedora/repository/$pid");
}

/**
 * Returns a formatted table listing all members of the collection
 * defined by the $collection_pid parameter
 * @param string $collection_pid
 * @return array
 */
function islandora_collectionPolicy_table($pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
  module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
  $collection_policy = CollectionPolicy::loadFromCollection($pid, TRUE);
  $current_models = $collection_policy->getContentModels();
  $current_vals = array();
  foreach ($current_models as $cm) {
    $current_vals[$cm->pid]['namespace'] = substr($cm->pid_namespace, 0, strpos($cm->pid_namespace, ":"));
    $current_vals[$cm->pid]['prompt'] = $cm->name;
  }

  $isRestricted = variable_get('fedora_namespace_restriction_enforced', TRUE);
  if ($isRestricted) {
    $permitted = array();
    $allowed_namespaces = explode(": ", variable_get('fedora_pids_allowed', 'default: demo: changeme: islandora: books: newspapers:'));
    foreach ($allowed_namespaces as $allowed_namespace) {
      $allowed_namespace = str_replace(':', '', $allowed_namespace);
      $permitted[$allowed_namespace] = trim($allowed_namespace);
    }
  }

  $keys = array();
  $allowed_content_models = array();
  $default_namespace = substr($pid, 0, strpos($pid, ":"));
  $collection_policy = CollectionPolicy::loadFromCollection($pid);
  $content_models = get_content_models_as_option_array();

  // results are returned with name appended to pid.  We strip the name out here
  foreach ($content_models as $pid => $string) {
    $string_parts = explode('~', $string);
    $content_models[$pid] = $string_parts[0];
    $keys[$pid] = '';
  }

  if ($collection_policy) {
    $supported_collection_models = $collection_policy->getContentModels();
  }

  foreach ($supported_collection_models as $current) {
    $allowed_content_models[$current->pid] = $current->pid;
  }
  //move current content models to the top of the display
  foreach ($content_models as $content_model => $value) {
    if (in_array($content_model, $allowed_content_models)) {
      $content_models = array($content_model => $value) + $content_models;
    }
  }

  $table = array(
    '#header' => array(theme('table_select_header_cell'), t('Content Model Pid'), 'Namespace', t('Prompt')),
    '#theme' => 'islandora_collectionPolicy_table',
    '#tree' => TRUE,
    'rows' => array(),
    'selections' => array(
      '#type' => 'checkboxes',
      '#options' => $keys,
      '#default_value' => $allowed_content_models,
    ),
  );
  $rows = &$table['rows'];
  if (empty($content_models)) {
    return;
  }
  foreach ($content_models as $pid => $string) {
    if ($pid == 'islandora:collectionCModel') {
      continue;
    }
    $namespace = $current_vals[$pid]['namespace'] ? $current_vals[$pid]['namespace'] : $default_namespace;
    $prompt = $current_vals[$pid]['prompt'] ? $current_vals[$pid]['prompt'] : t("New ") . $string;
    $cells = array();
    $cells['#pid'] = $pid;
    $cells['pid'] = array('#type' => 'item', '#value' => l($pid, 'fedora/repository/' . $pid) . "<br />$string", '#attributes' => array('title' => $string));
    $cells['namespace'] = array('#type' => 'textfield', '#default_value' => $namespace, '#size' => 8);
    if($isRestricted){
      $cells['namespace'] = array('#type' => 'select', '#options' => $permitted, '#default_value' => $namespace);
    }
    $cells['prompt'] = array('#type' => 'textfield', '#default_value' => $prompt);
    $cells['identifier'] = array('#type' => 'hidden', '#value' => $pid);
    $rows[] = $cells;
  }

  return $table;
}

/**
 * themes the form table.
 *
 * @param array $element Drupal Form Element.
 * @return string
 */
function theme_islandora_collectionPolicy_table(array $element) {
  $rows = array();
  foreach (element_children($element['rows']) as $child) {
    $setting = $element['rows'][$child];
    $pid = $setting['#pid'];

    $fields = array(
      drupal_render($element['selections'][$pid]) // First field is a checkbox
    );
    foreach (element_children($setting) as $property) {
      $field = $setting[$property];
      $fields[] = drupal_render($field);
    }
    $rows[] = array(
      'data' => $fields,
      'class' => isset($setting['#attributes']['class']) ? $setting['#attributes']['class'] : NULL
    );
  }
  $attributes = isset($element['#id']) ? array('id' => $element['#id']) : NULL;
  return theme_table($element['#header'], $rows, $attributes);
}

function get_blank_collection_policy() {
  return '<?xml version="1.0" encoding="UTF-8"?>
<collection_policy xmlns="http://www.islandora.ca" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" name="" xsi:schemaLocation="http://www.islandora.ca http://syn.lib.umanitoba.ca/collection_policy.xsd">
    <content_models>
    <content_model dsid="ISLANDORACM" name="Islandora Collection Model ~ islandora:collectionCModel" namespace="islandora:1" pid="islandora:collectionCModel"/>
    </content_models>
    <search_terms>
    </search_terms>
    <staging_area></staging_area>
    <relationship>isMemberOfCollection</relationship>
</collection_policy>';
}
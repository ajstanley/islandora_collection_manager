<?php

/**
 * Returns a formatted table listing all members of the collection
 * defined by the $collection_pid parameter
 * can be called for any function requiring a formatted table with selectable elements
 * @param string $collection_pid
 * @return array
 */
function islandora_collection_table($collection_pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');

  $query = "select \$object \$title from <#ri>
              where  (\$object <info:fedora/fedora-system:def/relations-external#isMemberOf> <info:fedora/$collection_pid>
              or \$object <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/$collection_pid>)
              and \$object <dc:title> \$title";
  $query = htmlentities(urlencode($query));
  $content = '';

  $url = variable_get('fedora_repository_url', 'http://localhost:8080/fedora/risearch');
  $url .= "?type=tuples&flush=TRUE&format=csv&limit=$limit&offset=$offset&lang=itql&stream=on&query=" . $query;
  $content .= do_curl($url);
  $results = explode("\n", $content);
  $lines = preg_replace('/info:fedora\/|"object","title"/', '', $results);
  $lines = array_values(array_filter($lines));
  $keys = array();
  $object = array();
  foreach ($lines as $line) {
    $line_parts = explode(',', $line);

    $objects[$line_parts[0]] = $line_parts[1];
    $keys[] = $line_parts[0];
  }

  $form['list'] = array(
    '#value' => $list,
  );

  $table = array(
    '#header' => array(theme('table_select_header_cell'), 'Thumbnail', 'PID', 'Label'),
    '#theme' => 'islandora_collection_management_form_table',
    '#tree' => TRUE,
    'rows' => array(),
    'selections' => array(
      '#type' => 'checkboxes',
      '#options' => array_fill_keys($keys, ''),
    ),
  );
  $rows = &$table['rows'];
  if (empty($objects)) {
    return;
  }

  foreach ($objects as $key => $object) {
    $alink_options = array(
      'attributes' => array('target' => '_blank', 'class' => 'fedora_image', 'title' => $key),
      'html' => TRUE,
    );
    $textOptions = array(
      'attributes' => array('target' => '_blank'),
    );

    $img = "<img src='$base_url/fedora/repository/$key/TN' width='28' height='42'>";
    $img = theme_image("$base_url/fedora/repository/$key/TN", '', NULL, array('width' => '20'), FALSE);
    $rows[] = array(
      '#pid' => $key,
      'img' => array('#value' => l($img, 'fedora/repository/' . $key, $alink_options)),
      'pid' => array('#value' => l($key, 'fedora/repository/' . $key, $textOptions)),
      'title' => array('#value' => $object),
    );
  }

  return $table;
}

/**
 * themes the form table.
 *
 * @param array $element Drupal Form Element.
 * @return string
 */
function theme_islandora_collection_management_form_table(array $element) {
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
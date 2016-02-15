<?

function theme_account($theme) {
  global $user, $site_name, $links, $menu;
  
  if ($user->id) { 

    function submission_number() {
      $result = db_query("SELECT COUNT(id) FROM stories WHERE status = 1");
      return ($result) ? db_result($result, 0) : 0;
    }

    function menu($name, $module) {
      global $menu;
      if ($module["menu"]) $menu = ($menu) ? array_merge($menu, $module["menu"]()) : $module["menu"]();
    }

    module_iterate("menu");

    // Display account settings:
    $content .= "<LI><A HREF=\"account.php?op=track&topic=comments\">track your comments</A></LI>\n";
    $content .= "<LI><A HREF=\"account.php?op=track&topic=stories\">track your stories</A></LI>\n";
    $content .= "<LI><A HREF=\"account.php?op=track&topic=site\">track $site_name</A></LI>\n";
    $content .= "<P>\n";
    $content .= "<LI><A HREF=\"submission.php\">submission queue</A> (<FONT COLOR=\"red\">". submission_number() ."</FONT>)</LI>\n";
    $content .= "<P>\n";
    $content .= "<LI><A HREF=\"account.php?op=edit&topic=user\">edit your information</A></LI>\n";
    $content .= "<LI><A HREF=\"account.php?op=edit&topic=site\">edit your preferences</A></LI>\n";
    $content .= "<LI><A HREF=\"account.php?op=edit&topic=content\">edit site content</A></LI>\n";
    $content .= "<P>\n";

    if (($user->id) && ($user->permissions == 1 || $user->id == 1)) {
      $content .= "<LI><A HREF=\"admin.php\">administrate ". $site_name ."</A></LI>\n";
      $content .= "<P>\n";
    }

    if ($menu && ksort($menu)) {
      foreach ($menu as $link=>$url) $content .= "<LI><A HREF=\"$url\">$link</A></LI>\n";
      $content .= "<P>\n";
    }

    $content .= "<LI><A HREF=\"account.php?op=logout\">logout</A></LI>\n";

    $theme->box("$user->userid's configuration", "$content");
  }
  else {
    $output .= "<CENTER>\n";
    $output .= " <FORM ACTION=\"account.php?op=Login\" METHOD=\"post\">\n";
    $output .= "  <P><B>Username:</B><BR><INPUT NAME=\"userid\" SIZE=\"15\"></P>\n";
    $output .= "  <P><B>Password:</B><BR><INPUT NAME=\"passwd\" SIZE=\"15\" TYPE=\"password\"></P>\n";
    $output .= "  <P><INPUT NAME=\"op\" TYPE=\"submit\" VALUE=\"Login\"></P>\n";
    $output .= "  <P><A HREF=\"account.php\">REGISTER</A></P>\n";
    $output .= " </FORM>\n";
    $output .= "</CENTER>\n";

    $theme->box("Login", $output);
  }
}


function theme_blocks($region, $theme) {
  global $id, $PHP_SELF, $user;
 
  switch (strtok($PHP_SELF, ".")) {
    case "/discussion":
      if ($user->id) $story = db_fetch_object(db_query("SELECT * FROM stories WHERE id = '$id'"));
      if ($story->status == 1) theme_moderation_results($theme, $story);
      else theme_new_headlines($theme);
      break;
    case "/index":
      if ($user->id) $result = db_query("SELECT * FROM blocks b LEFT JOIN layout l ON b.name = l.block WHERE (b.status = 2 OR (b.status AND l.user = '$user->id'))". (($region == "left" || $region == "right") ? ($region == "left" ? " AND b.region = 0" : " AND b.region = 1") : "") ." ORDER BY weight");
      else $result = db_query("SELECT * FROM blocks WHERE status = 2 ORDER BY weight");
      while ($block = db_fetch_object($result)) {
        $blocks = module_execute($block->module, "block");
        $theme->box($blocks[$block->offset]["subject"], $blocks[$block->offset]["content"]);     
      }
      break;
  }
}

function theme_morelink($theme, $story) {
  return ($story->article) ? "[ <A HREF=\"discussion.php?id=$story->id\"><FONT COLOR=\"$theme->hlcolor2\"><B>read more</B></FONT></A> | ". strlen($story->article) ." bytes | <A HREF=\"discussion.php?id=$story->id\"><FONT COLOR=\"$theme->hlcolor2\">". format_plural($story->comments, "comment", "comments") ."</FONT></A> ]" : "[ <A HREF=\"discussion.php?id=$story->id\"><FONT COLOR=\"$theme->hlcolor2\">". format_plural($story->comments, "comment", "comments") ."</FONT></A> ]";
}

function theme_moderation_results($theme, $story) {
  global $user;

  if ($user->id && $story->id && ($user->id == $story->author || user_getHistory($user->history, "s$story->id"))) {
    $result = db_query("SELECT * FROM users WHERE history LIKE '%s$story->id%'");
    while ($account = db_fetch_object($result)) {
      $output .= format_username($account->userid) ." voted `". user_getHistory($account->history, "s$story->id") ."'.<BR>";
    }
    
    $theme->box("Moderation results", ($output ? $output : "This story has not been moderated yet."));
  }
}

function theme_related_links($theme, $story) {
  // Parse story for <A HREF="">-tags:
  $text = stripslashes("$story->abstract $story->updates $story->article");
  while ($text = stristr($text, "<A HREF=")) {
    $link = substr($text, 0, strpos(strtolower($text), "</a>") + 4);
    $text = stristr($text, "</A>");
    if (!stristr($link, "mailto:")) $content .= "<LI>$link</LI>";
  }

  // Stories in the same category: 
  $content .= " <LI>More about <A HREF=\"search.php?category=". urlencode($story->category) ."\">$story->category</A>.</LI>";

  // Stories from the same author:
  if ($story->userid) $content .= " <LI>Also by <A HREF=\"search.php?author=". urlencode($story->userid) ."\">$story->userid</A>.</LI>";

  $theme->box("Related links", $content);
}

function theme_comment_moderation($comment) {
  global $user, $comment_votes;
  
  if ($user->id && $user->userid != $comment->userid && !user_getHistory($user->history, "c$comment->cid")) {
    $output .= "<SELECT NAME=\"moderate[$comment->cid]\">\n";
    foreach ($comment_votes as $key=>$value) $output .= " <OPTION VALUE=\"$value\">$key</OPTION>\n";
    $output .= "</SELECT>\n";
  }
  else {
    $output .= "<TABLE BORDER=\"0\" CELLSPACING=\"1\" CELLPADDING=\"1\"><TR><TD>score:</TD><TD>". format_data($comment->score) ."</TD></TR><TR><TD>votes:</TD><TD>". format_data($comment->votes) ."</TR></TABLE>";
  }

  return $output;
}

function theme_new_headlines($theme, $num = 10) {
  global $user;

  $content = "";
  $result = db_query("SELECT id, subject FROM stories WHERE status = 2 ORDER BY id DESC LIMIT $num");
  while ($story = db_fetch_object($result)) $content .= "<LI><A HREF=\"discussion.php?id=$story->id\">". check_output($story->subject) ."</A></LI>\n";
  $content .= "<P ALIGN=\"right\">[ <A HREF=\"search.php\"><FONT COLOR=\"$theme->hlcolor2\">more</FONT></A> ]</P>";
  $theme->box("Latest headlines", $content);
}

function theme_old_headlines($theme, $num = 10) {
  global $user;
  
  if ($user->stories) $result = db_query("SELECT id, subject, timestamp FROM stories WHERE status = 2 ORDER BY timestamp DESC LIMIT $user->stories, $num");
  else $result = db_query("SELECT id, subject, timestamp FROM stories WHERE status = 2 ORDER BY timestamp DESC LIMIT $num, $num");

  while ($story = db_fetch_object($result)) {    
    if ($time != date("F jS", $story->timestamp)) {
      $content .= "<P><B>". date("l, M jS", $story->timestamp) ."</B></P>\n";
      $time = date("F jS", $story->timestamp);
    }
    $content .= "<LI><A HREF=\"discussion.php?id=$story->id\">". check_output($story->subject) ."</A></LI>\n";
  }
  $content .= "<P ALIGN=\"right\">[ <A HREF=\"search.php\"><FONT COLOR=\"$theme->hlcolor2\">more</FONT></A> ]</P>";

  $theme->box("Older headlines", $content);
}


?>
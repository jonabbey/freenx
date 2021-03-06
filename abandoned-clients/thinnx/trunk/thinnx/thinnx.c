/* thinnx.c -- a simple client for FreeNX
 *
 * Copyright (C) 2005 Instituto de Estudos e Pesquisas dos Trabalhadores
 * no Setor Energ�tico, Marco Sinhoreli and Gustavo Noronha Silva
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/select.h>

#include <gtk/gtk.h>
#include <glib.h>

#define CONFFILE "/etc/thinnx/thinnx.conf"

#ifndef BINDIR
#define BINDIR "/usr/bin"
#endif

#ifndef XAUTHBINDIR
#define XAUTHBINDIR "/usr/X11R6/bin"
#endif
  
const gchar *homedir = NULL;
gchar *buffer = NULL;

void
flush_buffer (gchar *buffer)
{
  if (buffer == NULL)
    return;

  g_print (buffer);
  g_free (buffer);
  buffer = NULL;
}

gchar*
read_code (gint fd)
{
  size_t bytes_read;
  gchar *buffer = g_malloc0 (sizeof(gchar)*8);

  bytes_read = read (fd, buffer, 7);
  if (bytes_read == -1)
    return NULL;

  return buffer;
}

gchar*
read_line (gint fd)
{
  fd_set rfds;
  struct timeval tv;

  gchar *buffer = g_malloc (sizeof(gchar)*2);
  gchar tmp[1];
  gint count;

  gint retval;

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  tv.tv_sec = 3;
  tv.tv_usec = 0;

  buffer[0] = '\0';
  buffer[1] = '\0';

  for (count = 2; tmp[0] != '\n'; count++)
    {
      retval = select (fd+1, &rfds, NULL, NULL, &tv);

      if (retval == 0 || retval == -1)
	break;
      
      if (read (fd, tmp, 1) != 1)
	break;

      buffer = g_realloc (buffer, sizeof(gchar)*count);
      buffer[count - 2] = tmp[0];
    }
  buffer[count - 2] = '\0';

  if (buffer[0] == '\0')
    return NULL;

  return buffer;
}

gchar*
read_chars (gint fd, gint count)
{
  gchar *buffer = NULL;

  buffer = g_malloc0 (sizeof(gchar)*(count+1));

  read (fd, buffer, count);

  return buffer;
}

void
write_line (gint fd, gchar *line)
{
  gchar *buffer;

  buffer = g_strdup_printf ("%s\n", line);
  write (fd, buffer, strlen (buffer));
  flush_buffer (buffer);
}

void
drop_chars (gint fd, gint count)
{
  gchar *buffer;

  buffer = read_chars (fd, count);
  flush_buffer (buffer);
}

void
drop_line (gint fd)
{
  gchar *buffer = read_line (fd);
  flush_buffer (buffer);
}

void
drop_lines (gint fd, gint max)
{
  gint count;

  for (count = 0; count < max; count++)
    drop_line (fd);
}

void
protocol_error (gchar *msg)
{
  flush_buffer (buffer);
  g_critical ("Protocol Error!: %s", msg);
  exit (1);
}

void
quit_prog (GtkWidget *w, gpointer data)
{
  exit (0);
}

void
input_dialog (gchar **user, gchar **pass)
{
  GtkWidget *dialog;

  GdkColormap *colormap;
  GtkStyle *style;
  GdkBitmap *mask;
  GdkPixmap *pixmap;
  GtkWidget *image;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  GtkWidget *user_entry;
  GtkWidget *pass_entry;

  GtkWidget *connect_button;

  dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#if GTK_MAJOR_VERSION == 2
  gtk_window_set_decorated (GTK_WINDOW(dialog), FALSE);
#endif
  gtk_signal_connect (GTK_OBJECT(dialog), "delete-event",
		      GTK_SIGNAL_FUNC(quit_prog), NULL);
  gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_container_set_border_width (GTK_CONTAINER(dialog), 12);
  
  vbox = gtk_vbox_new (FALSE, 26);
  gtk_container_add (GTK_CONTAINER(dialog), vbox);

  style = gtk_widget_get_style (dialog);
  colormap = gtk_widget_get_colormap(dialog);
  pixmap = gdk_pixmap_colormap_create_from_xpm (dialog->window, colormap, 
						&mask, NULL,
						"/etc/icons/thinnx.xpm");
  image = gtk_pixmap_new (pixmap, mask);
  gtk_box_pack_start_defaults (GTK_BOX(vbox), image);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_box_pack_start_defaults (GTK_BOX(vbox), hbox);

  label = gtk_label_new ("Login:");
  gtk_box_pack_start_defaults (GTK_BOX(hbox), label);

  user_entry = gtk_entry_new ();
  gtk_box_pack_start_defaults (GTK_BOX(hbox), user_entry);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_box_pack_start_defaults (GTK_BOX(vbox), hbox);

  label = gtk_label_new ("Senha:");
  gtk_box_pack_start_defaults (GTK_BOX(hbox), label);

  pass_entry = gtk_entry_new ();
  gtk_signal_connect (GTK_OBJECT(pass_entry), "activate",
		      GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_entry_set_visibility (GTK_ENTRY(pass_entry), FALSE);
  gtk_box_pack_start_defaults (GTK_BOX(hbox), pass_entry);

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_box_pack_start_defaults (GTK_BOX(vbox), hbox);

  connect_button = gtk_button_new_with_label ("Conectar!");
  gtk_signal_connect (GTK_OBJECT(connect_button), "clicked",
		      GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_box_pack_start_defaults (GTK_BOX(hbox), connect_button);

  gtk_widget_show_all (dialog);

  gtk_widget_grab_focus (user_entry);

  gtk_main ();

  *user = g_strdup (gtk_entry_get_text (GTK_ENTRY(user_entry)));
  *pass = g_strdup (gtk_entry_get_text (GTK_ENTRY(pass_entry)));

  gtk_widget_destroy (dialog);
  
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

void
message_dialog (gchar *message)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;

  GdkColormap *colormap;
  GtkStyle *style;
  GdkBitmap *mask;
  GdkPixmap *pixmap;
  GtkWidget *image;

  GtkWidget *label;
  GtkWidget *button;

  dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#if GTK_MAJOR_VERSION == 2
  gtk_window_set_decorated (GTK_WINDOW(dialog), FALSE);
#endif
  gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_container_set_border_width (GTK_CONTAINER(dialog), 12);
  
  vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER(dialog), vbox);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_box_pack_start_defaults (GTK_BOX(vbox), hbox);

  style = gtk_widget_get_style (dialog);
  colormap = gtk_widget_get_colormap(dialog);
  pixmap = gdk_pixmap_colormap_create_from_xpm (dialog->window, colormap, 
						&mask, NULL,
						"/etc/icons/error.xpm");
  image = gtk_pixmap_new (pixmap, mask);
  gtk_box_pack_start_defaults (GTK_BOX(hbox), image);

  label = gtk_label_new (message);
  gtk_box_pack_start_defaults (GTK_BOX(hbox), label);

  button = gtk_button_new_with_label ("Fechar");
  gtk_signal_connect (GTK_OBJECT(button), "clicked",
		      GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_box_pack_start_defaults (GTK_BOX(vbox), button);

  gtk_widget_show_all (dialog);

  gtk_widget_grab_focus (button);

  gtk_main ();

  gtk_widget_destroy (dialog);
  
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

gchar*
get_info_after_char (gchar *buffer, gchar c)
{
  gchar *split_str;
  gchar *retval;
  
  printf ("%s", buffer);

  split_str = buffer+(strlen(buffer) - 1);
  while (*(split_str-1) != c)
    *split_str--;

  retval = g_strdup (split_str);
  g_strstrip (retval);
  
  return retval;
}

gchar*
get_info_after_colon (gchar *buffer)
{
  return get_info_after_char (buffer, ':');
}

gchar*
get_restore_id (gchar *line)
{
  return get_info_after_char (line, '\t');
}

/* Handle session start information */  
gchar *
get_session_info (gint out, gchar *code)
{
  gchar *buffer;
  gchar *retval;

  buffer = read_code (out);
  if (!strcmp (buffer, code))
    {
      flush_buffer (buffer);
      buffer = read_line (out);
      retval = get_info_after_colon (buffer);
      g_free (buffer);
      return retval;
    }

  g_warning ("Waiting for %s, got %s\n", code, buffer);
  protocol_error ("session info!");
  return NULL;
}

int
main (int argc, char **argv)
{
  gchar *confdir;

  gchar *host = g_strdup ("localhost");
  gchar *port = g_strdup ("22");
  gchar *sshkey = NULL;

  gchar *user = NULL;
  gchar *password = NULL;

  gchar *session = g_strdup ("thinnx");
  gchar *type = g_strdup ("unix-gnome");
  gchar *cookie = NULL;

  gboolean use_ssl = 0;

  gchar *link = g_strdup ("adsl");
  gchar *kbdtype = g_strdup ("pc104/us");

  gchar *geometry = g_strdup ("fullscreen");
  gchar *screeninfo = NULL;

  gchar *session_id = NULL;
  gchar *session_display = NULL;
  gchar *pcookie = NULL;
  
  gchar **nxssh_argv = (gchar**) g_malloc (sizeof(gchar*) * 9);

  gchar *restore_id = NULL;

  pid_t pid;

  int parent_pipe[2];	/* For talking to the parent */
  int child_pipe[2];	/* For talking to the child */

  gint in = 0, out = 0;

  gtk_init (&argc, &argv);

  homedir = g_get_home_dir ();
  confdir = g_strdup_printf ("%s/.nx/", homedir);
  sshkey = g_strdup_printf ("%s/.ssh/id_dsa", homedir);

  {
    struct stat info;
    
    if (stat (confdir, &info) == -1)
      {
      if (mkdir (confdir, 0777) == -1)
	g_critical ("Could not create directory %s: %s\n", 
		    confdir, strerror (errno));
      }
  }    

  {
#if GTK_MAJOR_VERSION == 2
    GdkScreen *screen;

    screen = gdk_screen_get_default ();
    
    screeninfo = g_strdup_printf ("%dx%dx%d+render", 
				  gdk_screen_get_width (screen),
				  gdk_screen_get_height (screen),
				  gdk_visual_get_best_depth ());
#else
    screeninfo = g_strdup_printf ("%dx%dx%d+render",
				  gdk_screen_width (),
				  gdk_screen_height (),
				  gdk_visual_get_best_depth ());
#endif
  }

  { /* get X authentication cookie information */
    FILE *xauth_output;
    gchar xauth[256] = {0};

    gchar *tmp = NULL;
    gchar **tmpv = NULL;
    gchar *display = NULL;

    /* avoid problems with "network" DISPLAY's */
    display = g_strdup (getenv ("DISPLAY"));
    tmpv = g_strsplit (display, ":", 3);
    g_free (display);
  
    display = g_strdup_printf (":%s", tmpv[1]);
    g_strfreev (tmpv);

    /* get the authorization token */
    tmp = g_strdup_printf (XAUTHBINDIR"/xauth list %s | "
			   "grep 'MIT-MAGIC-COOKIE-1' | "
			   "cut -d ' ' -f 5",
			   display);

    if ((xauth_output = popen (tmp, "r")) == NULL)
      {
	g_critical ("Failed to obtain xauth key: %s", strerror(errno));
	exit (1);
      }

    fread (xauth, sizeof(char), 256, xauth_output);
    xauth[strlen(xauth) - 1] = '\0';
    pclose (xauth_output);
    g_free (tmp);

    if (!strcmp (xauth, ""))
      {
	message_dialog ("N�o foi poss�vel obter um cookie de autentica��o\n"
			"do servidor X. Imposs�vel continuar.");
	exit (1);
      }

    cookie = g_strdup (xauth);
  }


  { /* read configuration file */
    FILE *fconf;
    gint fconf_fd;
    gchar **tmp, *key, *value;
    struct stat info;
    
    if (stat (CONFFILE, &info) == -1)
      {
	g_warning ("WARNING: Could not stat %s: %s.\n", 
		   CONFFILE, strerror (errno));
      }

    fconf = fopen (CONFFILE, "r");
    if (fconf == NULL)
      {
	g_critical ("Could not open %s: %s.\n", 
		    CONFFILE, strerror (errno));
      }
    else
      {
	fconf_fd = fileno (fconf);

	while (!feof (fconf))
	  {

	    buffer = read_line (fconf_fd);
	    if (!buffer)
	      break;

	    /* remove comments */
	    tmp = g_strsplit (buffer, "#", 2);
	    g_free (buffer);

	    buffer = g_strdup (tmp[0]);
	    g_strfreev (tmp);

	    /* check if we still have a key/value pair */
	    tmp = g_strsplit (buffer, "=", 2);
	    g_free (buffer);
	    if (tmp[1] == NULL || tmp[0] == NULL)
	      {
		g_strfreev (tmp);
		continue;
	      }

	    key = tmp[0];
	    value = tmp[1];

	    g_strstrip (key);
	    g_strstrip (value);

	    if (!strcmp ("host", key))
	      {
		g_free (host);
		host = g_strdup (value);
	      }
	    else if (!strcmp ("port", key))
	      {
		g_free (port);
		port = g_strdup (value);
	      }
	    else if (!strcmp ("sshkey", key))
	      {
		g_free (sshkey);
		sshkey = g_strdup (value);
	      }
	    else if (!strcmp ("session", key))
	      {
		g_free (session);
		session = g_strdup (value);
	      }
	    else if (!strcmp ("ssl", key))
	      {
		if (!strcmp (value, "yes"))
		    use_ssl = 1;
	      }
	    else if (!strcmp ("type", key))
	      {
		g_free (type);
		type = g_strdup (value);
	      }
	    else if (!strcmp ("link", key))
	      {
		g_free (link);
		link = g_strdup (value);
	      }
	    else if (!strcmp ("kbdtype", key))
	      {
		g_free (kbdtype);
		kbdtype = g_strdup (value);
	      }
	    else if (!strcmp ("geometry", key))
	      {
		g_free (geometry);
		geometry = g_strdup (value);
	      }
	    else
	      g_warning ("Unknown option in %s: %s=%s\n", 
			 CONFFILE, key, value);

	    g_strfreev (tmp);
	  }
	fclose (fconf);
      }
  }

  /* grab auth information from the user before anything else */
  input_dialog (&user, &password);

  if (!strcmp (user, "root"))
    {
      message_dialog ("O usu�rio root n�o pode entrar por aqui!");
      exit (1);
    }

  pipe (parent_pipe);
  pipe (child_pipe);

  pid = fork ();
  if (pid == -1)
    {
      g_critical ("Could not fork!\n");
      exit (1);
    }
  else if (pid == 0)
    {
      close (child_pipe[1]);
      dup2 (child_pipe[0], STDIN_FILENO);
      dup2 (parent_pipe[1], STDOUT_FILENO);

      nxssh_argv[0] = g_strdup (BINDIR"/nxssh");
      nxssh_argv[1] = g_strdup ("-nx");
      nxssh_argv[2] = g_strdup_printf ("-p%s", port);
      nxssh_argv[3] = g_strdup ("-i");
      nxssh_argv[4] = g_strdup (sshkey);
      nxssh_argv[5] = g_strdup_printf ("nx@%s", host);
      nxssh_argv[6] = g_strdup ("-2");
      nxssh_argv[7] = g_strdup ("-S");
      nxssh_argv[8] = NULL;

      execv (nxssh_argv[0], nxssh_argv);
    }
  else
    {
      close(parent_pipe[1]);

      out = parent_pipe[0];
      in = child_pipe[1];

      /* Handle initial hand-shaking */
      {
	gboolean ssh_authed = FALSE;

	while (!ssh_authed)
	  {
	    buffer = read_code (out);
	    if (!strcmp (buffer, "NX> 205"))
	      {
		flush_buffer (buffer);
		drop_line (out);
		drop_line (out);
		drop_chars (out, 56);
		write_line (in, "yes");
		drop_line (out);
		drop_line (out);

		/* buffer != NULL? but why?! */
		buffer = NULL;
	      }
	    else if (!strcmp (buffer, "NX> 208"))
	      { 
		/* OK, authenticating... */
	      }
	    else if (!strcmp (buffer, "NX> 203") || 
		     (!strcmp (buffer, "NX> 285")) ||
		     (!strcmp (buffer, "NX> 200")) ||
		     (!strcmp (buffer, "NX> 202")))
	      {
		/* ignored stderr */
	      }
	    else if (!strncmp (buffer, "nxssh", 5))
	      {
		gchar *msg;

		flush_buffer (buffer);
		buffer = read_line (out);
		msg = get_info_after_colon (buffer);
		g_free (buffer);

		if (!strcmp (msg, "Name or service not known"))
		  message_dialog ("N�o foi poss�vel resolver o nome do servidor.");
		else if (!strcmp (msg, "Connection refused"))
		  message_dialog ("A conex�o foi recusada!\n"
				  "Verifique a porta.");
		else if (!strcmp (msg, "Connection timed out"))
		  message_dialog ("Tempo limite da conex�o expirou!\n"
				  "Verifique servidor e porta.");

		flush_buffer (msg);
		fprintf (stderr, "\n");
		exit (1);
	      }
	    else if (!strcmp (buffer, "NX> 204"))
	      {
		message_dialog ("Falha na autentica��o inicial!\n"
				"Confira a chave privada.");
		g_critical ("Failed to authenticate to SSH using the public key!\n");
		exit (1);
	      }
	    else if (!strcmp (buffer, "HELLO N"))
	      {
		/* OK, time to say HELLO! */
		ssh_authed = TRUE;
	      }
	    else
	      protocol_error ("problems waiting for HELLO");

	    flush_buffer (buffer);

	    buffer = read_line (out);
	    flush_buffer (buffer);
	  }
      }

      /* Handle HELLO */
      buffer = read_code (out);
      if (!strcmp (buffer, "NX> 105"))
	{
	  flush_buffer (buffer);
	  drop_chars (out, 1);
	  write_line (in, "HELLO NXCLIENT - Version 1.4.0");
	  drop_line (out);
	}
      else
	protocol_error ("problems during HELLO");

      /* Handle Login */
      buffer = read_code (out);
      if (!strcmp (buffer, "NX> 134"))
	{
	  flush_buffer (buffer);
	  buffer = read_line (out);
	  flush_buffer (buffer);
	}
      else
	protocol_error ("HELLO failed?");

      buffer = read_code (out);
      if (!strcmp (buffer, "NX> 105"))
	{
	  flush_buffer (buffer);
	  drop_chars (out, 1);
	  write_line (in, "login");
	  drop_line (out);
	}
      else
	protocol_error ("No login? How come!");

      buffer = read_code (out);
      if (!strcmp (buffer, "NX> 101"))
	{
	  flush_buffer (buffer);
	  drop_chars (out, 7);
	  write_line (in, user);
	  drop_line (out);
	}
      else
	protocol_error ("who took my login prompt away?");

      buffer = read_code (out);
      if (!strcmp (buffer, "NX> 102"))
	{
	  flush_buffer (buffer);
	  drop_chars (out, 11);
	  write_line (in, password);
	  drop_line (out);
	}
      else
	protocol_error ("where is my password prompt?");

      buffer = read_code (out);
      if (!strcmp (buffer, "NX> 103"))
	{
	  flush_buffer (buffer);
	  drop_line (out);
	}
      else
	{
	  flush_buffer (buffer);
	  message_dialog ("Login ou senha incorretos!");
	  exit (1);
	}

      buffer = read_code (out);
      if (!strcmp (buffer, "NX> 105"))
	{
	  gchar *m;

	  flush_buffer (buffer);
	  drop_chars (out, 1);

	  m = g_strdup_printf ("list %s", user);
	  write_line (in, m);
	  g_free (m);

	  drop_lines (out, 5);

	  while (1)
	    {
	      buffer = read_code (out);
	      if (!strcmp (buffer, "NX> 105"))
		{
		  flush_buffer (buffer);
		  drop_chars (out, 1);
		  break;
		}

	      flush_buffer (buffer);
	      buffer = read_line (out);
	      restore_id = get_restore_id (buffer);
	    }
	}
      else
	protocol_error ("session startup, buddy, I don't want problems!");
      

      {
	gchar *cmdline;
	
	if (!restore_id)
	  cmdline = g_strdup_printf ("startsession --session=\"%s\" --type=\"%s\" --cache=\"8M\" --images=\"32M\" --cookie=\"%s\" --link=\"%s\" --kbtype=\"%s\" --nodelay=\"1\" --backingstore=\"never\" --geometry=\"%s\" --media=\"0\" --agent_server=\"\" --agent_user=\"\" --agent_password=\"\" --screeninfo=\"%s\" --encryption=\"%d\"", session, type, cookie, link, kbdtype, geometry, screeninfo, use_ssl);
	else
	  cmdline = g_strdup_printf ("restoresession --session=\"%s\" --type=\"%s\" --cache=\"8M\" --images=\"32M\" --cookie=\"%s\" --link=\"%s\" --kbtype=\"%s\" --nodelay=\"1\" --backingstore=\"never\" --geometry=\"%s\" --media=\"0\" --agent_server=\"\" --agent_user=\"\" --agent_password=\"\" --screeninfo=\"%s\" --encryption=\"%d\" --id=\"%s\"", session, type, cookie, link, kbdtype, geometry, screeninfo, use_ssl, restore_id);
	
	write_line (in, cmdline);
	g_free (cmdline);
	cmdline = NULL;
	
	drop_lines (out, 4);
      }

      session_id = get_session_info (out, "NX> 700");
      session_display = get_session_info (out, "NX> 705");
      drop_line (out); /* 703 (session type) */
      pcookie = get_session_info (out, "NX> 701");
      drop_line (out); /* 702 proxy ip */
      drop_line (out); /* 706 agent cookie */
      drop_line (out); /* 704 session cache */
      {
	gchar *tmp = get_session_info (out, "NX> 707"); /* 707 ssl tunneling */
	
	use_ssl = atoi (tmp);
	g_free (tmp);
      }
      drop_line (out); /* 710 session status: running */
      drop_line (out); /* 1002 commit */
      drop_line (out); /* 1006 session status: running */

      read_code (out);
      drop_chars (out, 1);

      /* now prepare to run nxproxy */
      {
	FILE *options;
		
	gchar *dirname;
	gchar *fname;
	gchar *cmdline;
		
	dirname = g_strdup_printf ("%s/.nx/S-%s", homedir, session_id);
	fname = g_strdup_printf ("%s/options", dirname);
		
	g_print ("Dir: %s\nFname: %s\n", dirname, fname);
		
	if (mkdir (dirname, 0777) == -1)
	  {
	    /* BOMB or handle 'directory already exists' */
	  }
	g_free (dirname);
		
	if (use_ssl)
	  buffer = g_strdup_printf ("cookie=%s,root=%s/.nx,session=%s,id=%s,listen=%d:%s", pcookie, homedir, session, session_id, 8008, session_display);
	else
	  buffer = g_strdup_printf ("cookie=%s,root=%s/.nx,session=%s,id=%s,connect=%s:%s", pcookie, homedir, session, session_id, host, session_display);
		
	options = fopen (fname, "w");
	fwrite (buffer, sizeof(char), strlen (buffer), options);
	fclose (options);
		
	g_free (buffer);
		
	cmdline = g_strdup_printf (BINDIR"/nxproxy -S options=%s:%s", fname, session_display);
	system (cmdline);
	g_free (cmdline);
		
	g_free (fname);
      }
    }

  write_line (in, "bye");
  drop_line (out);

  if (use_ssl)
    write_line (in, "switch");

  drop_line (out);
  drop_line (out);
  drop_line (out);

  return 0;
}

/* Gobby - GTK-based collaborative text editor
 * Copyright (C) 2008-2015 Armin Burgmeier <armin@arbur.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "commands/file-tasks/task-open.hpp"
#include "util/i18n.hpp"

Gobby::TaskOpen::TaskOpen(FileCommands& file_commands,
                          const Glib::RefPtr<Gio::File>& file):
	Task(file_commands), m_file(file)
{
}

Gobby::TaskOpen::~TaskOpen()
{
	if(m_handle != get_status_bar().invalid_handle())
		get_status_bar().remove_message(m_handle);
	get_document_location_dialog().hide();
}

void Gobby::TaskOpen::run()
{
	try
	{
		// TODO: Show DocumentLocationDialog with a
		// default name as long as the query is
		// running.
		m_file->query_info_async(
			sigc::mem_fun(*this, &TaskOpen::on_query_info),
			G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);

		m_handle = get_status_bar().add_info_message(
			Glib::ustring::compose(
				_("Querying \"%1\"..."),
				m_file->get_uri()));
	}
	catch(const Gio::Error& ex)
	{
		error(ex.what());
	}
}

void Gobby::TaskOpen::on_query_info(
	const Glib::RefPtr<Gio::AsyncResult>& result)
{
	get_status_bar().remove_message(m_handle);
	m_handle = get_status_bar().invalid_handle();

	DocumentLocationDialog& dialog = get_document_location_dialog();
	dialog.signal_response().connect(sigc::mem_fun(
		*this, &TaskOpen::on_location_response));

	try
	{
		Glib::RefPtr<Gio::FileInfo> info =
			m_file->query_info_finish(result);

		dialog.set_document_name(info->get_display_name());
		dialog.set_single_document_mode();
		dialog.present();
	}
	catch(const Gio::Error& ex)
	{
		error(ex.what());
	}
}

void Gobby::TaskOpen::on_location_response(int response_id)
{
	if(response_id == Gtk::RESPONSE_ACCEPT)
	{
		DocumentLocationDialog& dialog =
			get_document_location_dialog();

		InfBrowserIter iter;
		InfBrowser* browser = dialog.get_selected_directory(&iter);
		g_assert(browser != NULL);

		get_operations().create_document(
			browser, &iter, dialog.get_document_name(),
			get_preferences(), m_file, NULL);
	}

	finish();
}

void Gobby::TaskOpen::error(const Glib::ustring& message)
{
	get_status_bar().add_error_message(
		Glib::ustring::compose(
			_("Failed to open document \"%1\""),
			m_file->get_uri()),
		message);

	finish();
}

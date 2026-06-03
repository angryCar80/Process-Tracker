use std::process::Command;

use freedesktop_desktop_entry::{desktop_entries, get_languages_from_env};
use iced::event::{self, Event};
use iced::keyboard::key::{Key, Named};
use iced::widget::operation::{focus, snap_to, RelativeOffset};
use iced::widget::{column, mouse_area, scrollable, text, text_input};
use iced::{keyboard, Size};
use iced::{Element, Subscription};

struct AppEntry {
    name: String,
    exec: Vec<String>,
}

struct State {
    content: String,
    entries: Vec<AppEntry>,
    filtered: Vec<usize>,
    selected: usize,
    viewport_offset: f32,
    viewport_height: f32,
}

fn boot() -> (State, iced::Task<Message>) {
    (State::new(), focus(search_id()))
}

fn search_id() -> iced::widget::Id {
    iced::widget::Id::new("search")
}

fn scroll_id() -> iced::widget::Id {
    iced::widget::Id::new("scroll")
}

fn load_entries() -> Vec<AppEntry> {
    let locales = get_languages_from_env();
    let entries = desktop_entries(&locales);

    entries
        .iter()
        .filter(|entry| {
            entry.type_() == Some("Application")
                && !entry.no_display()
                && !entry.hidden()
                && entry.exec().is_some()
        })
        .filter_map(|entry| {
            let name = entry.name(&locales)?.to_string();
            let exec = entry.parse_exec().ok()?;
            Some(AppEntry { name, exec })
        })
        .collect()
}

impl State {
    fn new() -> Self {
        let entries = load_entries();
        let filtered: Vec<usize> = (0..entries.len()).collect();
        Self {
            entries,
            filtered,
            ..Default::default()
        }
    }
}

impl Default for State {
    fn default() -> Self {
        Self {
            content: String::new(),
            entries: Vec::new(),
            filtered: Vec::new(),
            selected: 0,
            viewport_height: 0.0,
            viewport_offset: 0.0,
        }
    }
}

#[derive(Debug, Clone)]
enum Message {
    InputChanged(String),
    SelectNext,
    SelectPrev,
    Activate,
    ViewportChanged(f32, f32),
    Exit,
}

fn relative_y(selected: usize, total: usize) -> f32 {
    if total <= 1 {
        0.0
    } else {
        selected as f32 / (total - 1) as f32
    }
}

fn subscription(_state: &State) -> Subscription<Message> {
    event::listen_with(|event, _status, _id| match event {
        Event::Keyboard(keyboard::Event::KeyPressed {
            key: Key::Named(Named::ArrowDown),
            ..
        }) => Some(Message::SelectNext),

        Event::Keyboard(keyboard::Event::KeyPressed {
            key: Key::Named(Named::ArrowUp),
            ..
        }) => Some(Message::SelectPrev),
        Event::Keyboard(keyboard::Event::KeyPressed {
            key: Key::Named(Named::Escape),
            ..
        }) => Some(Message::Exit),
        Event::Keyboard(keyboard::Event::KeyPressed {
            key: Key::Named(Named::Enter),
            ..
        }) => Some(Message::Activate),
        Event::Keyboard(keyboard::Event::KeyPressed {
            key: Key::Named(Named::Tab),
            modifiers,
            ..
        }) => {
            if modifiers.shift() {
                Some(Message::SelectPrev)
            } else {
                Some(Message::SelectNext)
            }
        }
        _ => None,
    })
}

fn theme(_state: &State) -> iced::Theme {
    iced::Theme::GruvboxDark
}

fn update(state: &mut State, message: Message) -> iced::Task<Message> {
    match message {
        Message::InputChanged(s) => {
            state.content = s;
            state.filtered = state
                .entries
                .iter()
                .enumerate()
                .filter(|(_, entry)| {
                    entry
                        .name
                        .to_lowercase()
                        .contains(&state.content.to_lowercase())
                })
                .map(|(i, _)| i)
                .collect();
            state.selected = 0;
            state.viewport_offset = 0.0;
            state.viewport_height = 0.0;
            iced::Task::none()
        }
        Message::SelectNext => {
            state.selected = (state.selected + 1).min(state.filtered.len().saturating_sub(1));
            let y = relative_y(state.selected, state.filtered.len());
            snap_to(scroll_id(), RelativeOffset { x: 0.0, y })
        }
        Message::SelectPrev => {
            state.selected = state.selected.saturating_sub(1);
            let y = relative_y(state.selected, state.filtered.len());
            snap_to(scroll_id(), RelativeOffset { x: 0.0, y })
        }
        Message::Activate => {
            if let Some(&idx) = state.filtered.get(state.selected) {
                let entry = &state.entries[idx];
                let args = entry.exec.clone();
                let _ = std::thread::spawn(move || {
                    if let Some(prog) = args.first() {
                        let _ = Command::new(prog).args(&args[1..]).spawn();
                    }
                });
                return iced::exit();
            }
            iced::Task::none()
        }
        Message::ViewportChanged(y, h) => {
            state.viewport_offset = y;
            state.viewport_height = h;
            iced::Task::none()
        }
        Message::Exit => iced::exit(),
    }
}

fn view(state: &State) -> Element<'_, Message> {
    let items: Vec<Element<'_, Message>> = state
        .filtered
        .iter()
        .copied()
        .enumerate()
        .map(|(pos, entry_idx)| {
            let label = &state.entries[entry_idx].name;
            if pos == state.selected {
                column![text(format!("> {label}"))].into()
            } else {
                column![text(format!("  {label}"))].into()
            }
        })
        .collect();

    column![
        text_input("Search apps...", &state.content)
            .id(search_id())
            .on_input(Message::InputChanged),
        mouse_area(
            scrollable(column(items))
                .id(scroll_id())
                .on_scroll(|vp| {
                    Message::ViewportChanged(vp.absolute_offset().y, vp.bounds().height)
                })
                .style(|_, _| scrollable::Style {
                    container: iced::widget::container::Style::default(),
                    gap: None,
                    vertical_rail: scrollable::Rail {
                        background: None,
                        border: iced::Border::default(),
                        scroller: scrollable::Scroller {
                            background: iced::Background::Color(iced::Color::TRANSPARENT,),
                            border: iced::Border::default(),
                        },
                    },
                    horizontal_rail: scrollable::Rail {
                        background: None,
                        border: iced::Border::default(),
                        scroller: scrollable::Scroller {
                            background: iced::Background::Color(iced::Color::TRANSPARENT,),
                            border: iced::Border::default(),
                        },
                    },
                    auto_scroll: scrollable::AutoScroll {
                        background: iced::Background::Color(iced::Color::TRANSPARENT,),
                        border: iced::Border::default(),
                        shadow: iced::Shadow::default(),
                        icon: iced::Color::TRANSPARENT,
                    },
                })
        )
        .on_scroll(|delta| match delta {
            iced::mouse::ScrollDelta::Lines { y, .. }
            | iced::mouse::ScrollDelta::Pixels { y, .. } => {
                if y > 0.0 {
                    Message::SelectPrev
                } else {
                    Message::SelectNext
                }
            }
        }),
    ]
    .height(iced::Fill)
    .into()
}

fn main() -> iced::Result {
    iced::application(boot, update, view)
        .theme(theme)
        .centered()
        .decorations(false)
        .level(iced::window::Level::AlwaysOnTop)
        .resizable(false)
        .window_size(Size::new(600.0, 400.0))
        .subscription(subscription)
        .run()
}

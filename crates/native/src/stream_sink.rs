#[cfg(feature = "rust_api")]
pub use rust_api::Sink;

#[cfg(feature = "dart_api")]
pub use dart_api::Sink;

#[cfg(feature = "rust_api")]
mod rust_api {
    use std::sync::mpsc;

    use derive_more::From;

    #[derive(Clone, From)]
    pub struct Sink<T>(mpsc::Sender<T>);

    impl<T> Sink<T> {
        pub fn send(&self, value: T) -> bool {
            self.0.send(value).is_ok()
        }
    }
}

#[cfg(feature = "dart_api")]
mod dart_api {
    use std::sync::Arc;

    use flutter_rust_bridge::{self as frb, support::IntoDart};

    /// [`flutter_rust_bridge`]'s [`StreamSink`] wrapper that is automatically
    /// closed once [`Drop`]ped.
    ///
    /// [`StreamSink`]: frb::StreamSink
    #[derive(Clone)]
    pub struct Sink<T: IntoDart>(Arc<frb::StreamSink<T>>);

    impl<T: IntoDart> Sink<T> {
        pub fn send(&self, value: T) -> bool {
            self.0.add(value)
        }
    }

    impl<T: IntoDart> From<frb::StreamSink<T>> for Sink<T> {
        fn from(val: frb::StreamSink<T>) -> Self {
            Self(Arc::new(val))
        }
    }

    impl<T: IntoDart> Drop for Sink<T> {
        fn drop(&mut self) {
            if let Some(sink) = Arc::get_mut(&mut self.0) {
                sink.close();
            }
        }
    }
}
